// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2004 Nicolas Rougier
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
// ========================================================================
//
// File          : $RCSfile$
// Revision      : $Revision$
// Revision date : $Date$
// Author(s)     : Nicolas Rougier
// Short         : 
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#include "support.h"

#include <algorithm>
#include <string>
#include <sstream>
#include <glib.h>
#include <sys/stat.h>
#include <utime.h>

#include "ui-authentication.h"
#include "ui-applet.h"
#include "ui-popup.h"
#include "imap4.h"
#include "nls.h"

// ========================================================================
//  base
// ========================================================================	
/**
 * Constructor. The mailbox for the IMAP protocol is created from scratch.
 *
 * @param biff Pointer to the instance of Gnubiff.
 */
Imap4::Imap4 (Biff *biff) : Mailbox (biff)
{
	value ("protocol", PROTOCOL_IMAP4);
	socket_   = new Socket (this);
	idleable_ = false;
	idled_    = false;
}

/**
 * Constructor. The mailbox for the IMAP protocol is created by taking the
 * attributes of the existing mailbox {\em other}.
 *
 * @param other Mailbox from which the attributes are taken.
 */
Imap4::Imap4 (const Mailbox &other) : Mailbox (other)
{
	value ("protocol", PROTOCOL_IMAP4);
	socket_   = new Socket (this);
	idleable_ = false;
	idled_    = false;
}

/// Destructor
Imap4::~Imap4 (void)
{
	delete socket_;
}

// ========================================================================
//  main
// ========================================================================	
/**
 * Make a note to start monitoring in a new thread. If there is already a note
 * or if we are in idle state nothing is done.
 *
 * @param delay Time (in seconds) to wait before the new thread will be
 *              created. If {\em delay} is zero (this is the default) the
 *              value of {\em delay()} is taken.
 */
void 
Imap4::threaded_start (guint delay)
{
	// Are we in idle state?
	if (idled_)
		return;

	// If no delay is given use internal delay
	if (!delay)
		delay = Mailbox::delay();

	Mailbox::threaded_start (delay);
}

/**
 * Method to be called by a new thread for monitoring the mailbox. The status
 * of the mailbox will be updated, new mails fetched and idle state entered
 * (if the server does allow this). Before exiting creating of a new thread
 * for monitoring is noted down.
 *
 * Remark: In this function all exceptions are catched that are thrown when
 * sending IMAP commands or receiving response from the server.
 */
void 
Imap4::start (void)
{
	if (!g_mutex_trylock (monitor_mutex_))
		return;

	try {
		start_checking ();
	}
	catch (imap_err& err) {
		// Catch all errors that are un-recoverable and result in
		// closing the connection, and resetting the mailbox status.
#if DEBUG
		g_warning ("[%d] Imap exception: %s", uin(), err.what());
#endif
		if (err.is_mailboxerror()) {
			status (MAILBOX_ERROR);
			unread_.clear ();
			seen_.clear ();
		}
		socket_->close ();

		idled_ = false;
	}

	update_applet();

	g_mutex_unlock (monitor_mutex_);

	threaded_start (delay());
}

/**
 * Connect to the mailbox, get unread mails and update mailbox status.
 * If the password for the mailbox isn't already known, it is obtained (if
 * possible). If the mailbox supports the "IDLE" command this function starts
 * idling once the mailbox status is known. When leaving this function gnubiff
 * will logout from the server.
 *
 * @exception imap_command_err
 *                     This exception is thrown when we get an unexpected
 *                     response.
 * @exception imap_dos_err
 *                     This exception is thrown when a DoS attack is suspected.
 * @exception imap_nologin_err
 *                     The server doesn't want us to login or the user doesn't
 *                     provide a password.
 * @exception imap_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Imap4::fetch (void) throw (imap_err)
{
	// Is there a password? Can we obtain it?
	if (!biff_->password(this)) throw imap_nologin_err();

	// Connection and authentification
	connect();

	// Set the mailbox status and get mails (if there is new mail)
	fetch_mails();

	// Start idling (if possible)
	if (idleable_)
		idle();

	// LOGOUT
	command_logout();
}

/**
 * Update the applet with any new information about this mailbox. This
 * includes new or removed mail, for a new mail count.
 */
void 
Imap4::update_applet(void)
{
	// Removed the below so notifications will queue up, instead
	// of potential notifications being missed.
	// if (!GTK_WIDGET_VISIBLE (biff_->popup()->get())) {
		gdk_threads_enter();
		biff_->applet()->update();
		gdk_threads_leave();
	// }
}

/**
 * A connection to the mailbox is established. If this can't be done then an
 * {\em imap_socket_err} is thrown. Otherwise gnubiff logins, checks
 * capabilities and selects the user chosen folder on the server.
 *
 * @exception imap_command_err
 *                     This exception is thrown when we get an unexpected
 *                     response.
 * @exception imap_dos_err
 *                     This exception is thrown when a DoS attack is suspected.
 * @exception imap_nologin_err
 *                     The server doesn't want us to login or the user doesn't
 *                     provide a password.
 * @exception imap_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Imap4::connect (void) throw (imap_err)
{
	// Resetting the tag counter
	reset_tag();

#ifdef DEBUG
	g_message ("[%d] Trying to connect to %s on port %d", uin(),
			   address().c_str(), port());
#endif

	// Determine authentication
	if (authentication() == AUTH_AUTODETECT) {
		guint prt = port();
		if (!use_other_port())
			prt = 993;
		if (!socket_->open (address(), prt, AUTH_SSL)) {
			if (!use_other_port())
				prt = 143;
			if (!socket_->open (address(), prt, AUTH_USER_PASS))
				throw imap_socket_err();
			else {
				port (prt);
				authentication (AUTH_USER_PASS);
				socket_->close();
			}
		}
		else {
			port (prt);
			authentication (AUTH_SSL);
			socket_->close();
		}
	}

	// Open socket
	if (!socket_->open (address(), port(), authentication(), certificate(), 3))
		throw imap_socket_err();

	// Set reads from the socket to time out.	We do this primarily for
	// the IDLE state.	However, this also prevents reads in general
	// from blocking forever on connections that have gone bad.	 We
	// don't let the timeout period be less then 60 seconds.
	socket_->set_read_timeout(delay() < 60 ? 60 : delay());

#ifdef DEBUG
	g_message ("[%d] Connected to %s on port %d", uin(),
			   address().c_str(), port());
#endif

	// Get server's response (maybe we get the CAPABILITY response code)
	ok_response_codes_.clear ();
	std::string line;
	readline (line);

	// CAPABILITY
	command_capability (true);

	// LOGIN
	command_login();

	// SELECT
	command_select();
}

/**
 * Get the first lines of the unread mails and update the status of the
 * mailbox.
 *
 * @exception imap_command_err
 *                     This exception is thrown when we get an unexpected
 *                     response.
 * @exception imap_dos_err
 *                     This exception is thrown when a DoS attack is suspected.
 * @exception imap_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Imap4::fetch_mails (void) throw (imap_err)
{
	// SEARCH NOT SEEN
	std::set<guint> buffer = command_searchnotseen ();

	// FETCH UID
	std::set<std::string> uid_set = command_fetchuid (buffer);

	// Get new mails one by one
	for (std::set<guint>::iterator i=buffer.begin(); i != buffer.end(); i++) {
		// Check if mail is already known
		std::string mailid = uidvalidity_ + msn_uid_[*i];
		if (new_mail (mailid))
			continue;

		// FETCH header information
		std::vector<std::string> mail = command_fetchheader (*i);

		// FETCH BODYSTRUCTURE
		PartInfo partinfo = command_fetchbodystructure (*i);

		// FETCH BODY
		command_fetchbody (*i, partinfo, mail);

		// Decode and parse mail
		if (partinfo.part_ != "")
			decode_body (mail, partinfo.encoding_);
		parse (mail, mailid);
	}
}

/**
 * Begin the IMAP idle mode.  This method will not return until
 * either we receive IMAP notifications (new mail...), or the server
 * terminates for some reason.
 *
 * @exception imap_command_err
 *                     This exception is thrown when we get an unexpected
 *                     response.
 * @exception imap_dos_err
 *                     This exception is thrown when a DoS attack is suspected.
 * @exception imap_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Imap4::idle (void) throw (imap_err)
{
	idled_ = true;
	gboolean sentdone=false; // "DONE\r\n" sent by command_idle()?

	// Currently we will never exit this loop unless an error occurs,
	// probably due to the loss of a connection.	Basically our loop is:
	// (update applet)->(wait in idle for mail change)->(Get Mail headers)
	while (true) {
		// When in idle state, we won't exit this thread function
		// so we have to update applet in the meantime
		update_mailbox_status ();
		update_applet ();

		if (timetag_)
			g_source_remove (timetag_);
		timetag_ = 0;

		// IDLE
		command_idle (sentdone);

		if (!sentdone)
			if (socket_->write (std::string("DONE\r\n")) != SOCKET_STATUS_OK)
				throw imap_socket_err();

		// Getting the acknowledgment
		waitfor_ack ();

		// Set mailbox status
		status (MAILBOX_CHECK);

		// Get mails
		fetch_mails ();
	}
	idled_ = false;
}

/**
 * Sending the IMAP command "CAPABILITY" and parsing the server's response.
 * The command "CAPABILITY" is sent to the server to get the supported
 * capabilities. Currently gnubiff recognizes the following capabilities:
 * \begin{itemize}
 *    \item IDLE: If the server has the IDLE capability, gnubiff uses the
 *          IDLE command instead of polling.
 *    \item LOGINDISABLED: The server wants us not to login.
 * \end{itemize}
 *
 * If {\em check_rc} is true first the response codes to untagged OK responses
 * are checked for a "CAPABILITY" response code. If this is found no command
 * is sent to the server. Instead the arguments to this response code are
 * evaluated.
 * 
 * @param     check_rc Whether to check the response codes first. The default
 *                     is false.
 * @exception imap_command_err
 *                     This exception is thrown when we get an unexpected
 *                     response.
 * @exception imap_dos_err
 *                     This exception is thrown when a DoS attack is suspected.
 * @exception imap_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Imap4::command_capability (gboolean check_rc) throw (imap_err)
{
	std::string line;

	// Check for CAPABILITY response code
	if (check_rc)
		if (ok_response_codes_.find("CAPABILITY") != ok_response_codes_.end())
			line = " " + ok_response_codes_["CAPABILITY"] + " ";

	// If no response code available send the command
	if (line.size() == 0) {
		// Sending the command
		sendline ("CAPABILITY");

		// Wait for "* CAPABILITY" untagged response
		waitfor_untaggedresponse (0, "CAPABILITY");
		line = " " + last_untagged_response_cont_ + " ";

		// Getting the acknowledgment
		waitfor_ack();
	}

	// Looking for supported capabilities
	idleable_ = use_idle () && (line.find (" IDLE ") != std::string::npos);


	if (line.find (" LOGINDISABLED ") != std::string::npos) {
		command_logout();
		throw imap_nologin_err();
	}

	// If we checked only the response code and didn't find all the
	// capabilities we are looking for (currently only IDLE), we send the
	// CAPABILITY command, maybe the server sends additional capabilities
	if (((idleable_ == false) && use_idle()) && check_rc)
		command_capability (false);
}

/**
 * Obtain the first lines of the body of the mail with sequence number
 * {\em msn}.
 *
 * @param     msn      Message sequence number of the mail
 * @param     partinfo Partinfo structure with information of the relevant
 *                     part of the mail as returned by
 *                     Imap4::command_fetchbodystructure().
 * @param     mail     Vector containing the mail's header lines (inclusive
 *                     the separating empty line).
 * @exception imap_command_err
 *                     This exception is thrown when we get an unexpected
 *                     response.
 * @exception imap_dos_err
 *                     This exception is thrown when a DoS attack is suspected.
 * @exception imap_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Imap4::command_fetchbody (guint msn, class PartInfo &partinfo,
						  std::vector<std::string> &mail) throw (imap_err)
{
	std::string line;

	// Do we have to get any plain text?
	if (partinfo.part_=="") {
		mail.push_back(std::string(_("[This mail has no \"text/plain\" part]")));
		return;
	}
	else if (partinfo.size_ == 0) {
		mail.push_back(std::string(""));
		return;
	}

	// Insert character set into header
	if (partinfo.charset_!="") {
		line = "Content-type: " + partinfo.mimetype_ + "; charset=";
		line+= partinfo.charset_;
		mail.insert (mail.begin(), line);
	}

	// Note: We are only interested in the first lines, there
	// are at most 1000 characters per line (see RFC 2821 4.5.3.1),
	// so it is sufficient to get at most 1000*min_body_lines
	// bytes.
	guint textsize = partinfo.size_;
	if (textsize > 1000 * biff_->value_uint ("min_body_lines"))
		textsize = 1000 * biff_->value_uint ("min_body_lines");
	std::stringstream textsizestr;
	textsizestr << textsize;

	// Send command
	line = "(BODY.PEEK[" + partinfo.part_ + "]<0." + textsizestr.str() + ">)";
	sendline ("FETCH", msn, line);

	// Wait for "* ... FETCH" untagged response (see RFC 3501 7.4.2)
	waitfor_untaggedresponse (msn, "FETCH");
			
#ifdef DEBUG
	g_print ("** Message: [%d] RECV(%s:%d): (message) ", uin(),
			 address().c_str(), port());
#endif
	// Read text
	guint lineno = 0;
	gint bytes = textsize + 3; // ")\r\n" at end of mail
	while ((bytes > 0) && (readline (line, false, true, false))) {
		bytes-=line.size()+1; // don't forget to count '\n'!
		if ((line.size() > 0)
			&& (lineno++ < biff_->value_uint ("min_body_lines"))) {
			mail.push_back (line.substr(0, line.size()-1));
#ifdef DEBUG
			g_print ("+");
#endif
		}
	}
#ifdef DEBUG
		g_print ("\n");
#endif
	if (bytes<0) throw imap_dos_err();
	// Remove ")\r" from last line ('\n' was removed before)
	mail.pop_back();
	if ((line.size()>1) && (line[line.size()-2]==')'))
		mail.push_back (line.substr(0, line.size()-2));
	else
		throw imap_command_err();

	// Getting the acknowledgment
	waitfor_ack();
}

/**
 * Decide which part from the mail with sequence number {\em msn} we are
 * interested in. This is done by sending IMAP command
 * "FETCH {\em msn} (BODYSTRUCTURE)" to the server and parsing the server's
 * response.
 *
 * @param     msn      Message sequence number of the mail
 * @return             Partinfo structure with information of the relevant
 *                     part of the mail.
 * @exception imap_command_err
 *                     This exception is thrown when we get an unexpected
 *                     response.
 * @exception imap_dos_err
 *                     This exception is thrown when a DoS attack is suspected.
 * @exception imap_socket_err
 *                     This exception is thrown if a network error occurs.
 */
class PartInfo 
Imap4::command_fetchbodystructure (guint msn) throw (imap_err)
{
	std::string line, response;
	guint nestlevel=0;

	// Send command
	sendline ("FETCH", msn, "(BODYSTRUCTURE)");

	// Wait for "* ... FETCH (BODYST..." untagged response (see RFC 3501 7.4.2)
	waitfor_untaggedresponse (msn, "FETCH", "(BODYSTRUCTURE (");

	// Get the whole response (may be multiline)
	response = last_untagged_response_cont_.substr (16); // "(BODYSTRUCTURE ("
	gint cnt = biff_->value_uint ("prevdos_imap4_multiline");
	while ((nestlevel=isfinished_fetchbodystructure(line,nestlevel))&&(cnt--)){
		readline (line, true, true, false);
		response += line.substr (0, line.size()-1); // trailing '\r'
	}
	if (cnt < 0) throw imap_dos_err();
	response=response.substr(0,response.size()-1); // trailing ')'

	// Get part of mail that contains "text/plain" (if any exists) and its
	// properties
	PartInfo partinfo;
	parse_bodystructure(response, partinfo);
#ifdef DEBUG
	g_print("** Message: [%d] Part=%s size=%d, encoding=%s, charset=%s\n",
			uin(), partinfo.part_.c_str(), partinfo.size_,
			partinfo.encoding_.c_str(),	partinfo.charset_.c_str());
#endif

	// Getting the acknowledgment
	waitfor_ack();

	return partinfo;
}

/**
 * Obtain some header information from the mail with sequence number {\em msn}.
 * The IMAP command "FETCH" is sent to the server in order to obtain the From,
 * Date and Subject of the mail. The last line of the returned header lines
 * should be empty.
 * 
 * @param     msn      Message sequence number of the mail
 * @return             C++ vector of C++ strings containing the header lines
 * @exception imap_command_err
 *                     This exception is thrown when we get an unexpected
 *                     response.
 * @exception imap_dos_err
 *                     This exception is thrown when a DoS attack is suspected.
 * @exception imap_socket_err
 *                     This exception is thrown if a network error occurs.
 */
std::vector<std::string> 
Imap4::command_fetchheader (guint msn) throw (imap_err)
{	
	// Start with an empty mail
	std::vector<std::string> mail;

	// Send command
	sendline ("FETCH", msn, "(BODY.PEEK[HEADER.FIELDS (DATE FROM SUBJECT)])");

	// Wait for "* ... FETCH" untagged response (see RFC 3501 7.4.2)
	waitfor_untaggedresponse(msn, "FETCH");
		
	// Date, From, Subject and an empty line
#ifdef DEBUG
	g_print ("** Message: [%d] RECV(%s:%d): (message) ", uin(),
			 address().c_str(), port());
#endif
	std::string line;
	gint cnt = 5 + biff_->value_uint ("prevdos_additional_lines");
	while ((readline (line, false, true, false)) && (cnt--)) {
		if (line.find (tag()) == 0)
			break;
		if (line.size() > 0) {
			mail.push_back (line.substr(0, line.size()-1));
#ifdef DEBUG
			g_print ("+");
#endif
		}
	}
#ifdef DEBUG
	g_print ("\n");
#endif
	// Did an error happen?
	if (cnt<0) throw imap_dos_err();
	if ((line.find (tag() + "OK") != 0) || (mail.size()<2))
		throw imap_command_err();
		
	// Remove the last line (should contain a closing parenthesis).
	// Note: We need the empty line before because it separates the
	// header from the mail text
	if ((mail[mail.size()-1]!=")") && (mail[mail.size()-2].size()!=0))
		throw imap_command_err();
	mail.pop_back();
	return mail;
}

/**
 * Obtain all the unique identifiers for messages with a sequence number in
 * the set {\em msn}. This is done by sending the IMAP command "FETCH" to the
 * server. This function also updates Imap4::msn_uid_.
 * 
 * @param     msn      Message sequence number of the mail
 * @return             Unique id of the mail
 * @exception imap_command_err
 *                     This exception is thrown when we get an unexpected
 *                     response.
 * @exception imap_dos_err
 *                     This exception is thrown when a DoS attack is suspected.
 * @exception imap_socket_err
 *                     This exception is thrown if a network error occurs.
 */
std::set<std::string> 
Imap4::command_fetchuid (std::set<guint> msn) throw (imap_err)
{
	std::set<std::string> uid;
	msn_uid_.clear ();

	// No messages at all
	if (!msn.size())
		return uid;

	std::stringstream ss;
	std::set<guint>::iterator i = msn.begin();
	ss << *(i++);
	while (i != msn.end())
		ss << "," << *(i++);

	// Send command
	sendline ("FETCH " + ss.str () + " (UID)");

	// Get all untagged responses that are of interest
	guint cnt = msn.size()+1; // prevent DoS
	while ((waitfor_ack_untaggedresponse ("FETCH" , "(UID ")) && (cnt--)) {
		// Get uid
		std::string line=last_untagged_response_cont_.substr (5);
		guint pos=line.find(")");
		if ((pos == 0) || (pos == std::string::npos)) throw imap_command_err();
		if (msn.find (last_untagged_response_msn_) == msn.end ())
			throw imap_command_err();
		uid.insert (line.substr (0, pos));
		msn_uid_[last_untagged_response_msn_] = line.substr (0, pos);
	}
	if (cnt <= 0) throw imap_dos_err();
	if (msn.size() != uid.size()) throw imap_command_err();

	return uid;
}

/**
 * Sending the IMAP command "IDLE" to the server.
 * This function enters into the idle mode by issueing the imap "IDLE"
 * command, then waits for notifications from the IMAP server.
 * With inactivity the socket read will timeout periodically waiting for server
 * notifications.  When the timeout occurs we simply issue the IMAP
 * "DONE" command then re-enter the idle mode again.  The timeout
 * occurs every {\em delay()} + 1 minute time.  We perform this timeout
 * operation so that we periodically test the connection to make sure it
 * is still valid, and to also keep the connection from being closed by
 * keeping the connection active.
 *
 * @param sentdone Reference to a boolean. When returning this is true if
 *                 "DONE" (for the last "IDLE") is already sent to the server,
 *                 and false if the caller still has to send "DONE".
 * @return         Returns the last line received from the IMAP server.
 * @exception imap_command_err
 *                 This exception is thrown when we get an unexpected response.
 * @exception imap_socket_err
 *                 This exception is thrown if a network error occurs.
 */
std::string 
Imap4::command_idle(gboolean &sentdone) throw (imap_err)
{
	gboolean idleRenew = false;	 // If we should renew the IDLE again.
	std::string line;
	do {
		idleRenew = false;
		sentdone = false;

		// IDLE
		sendline ("IDLE");
		
		// Read continuation response
		readline (line);
		if (line.find("+ ") != 0) throw imap_command_err();

		// Wait for new mail and block thread at this point
		gint status = readline_ignoreinfo (line, true, false, true);
		if (status == SOCKET_TIMEOUT) {
			// We timed out, so we want to loop, and issue IDLE again.
			idleRenew = true;

			if (socket_->write (std::string("DONE\r\n")) != SOCKET_STATUS_OK)
				throw imap_socket_err();
			sentdone=true;

			status = readline_ignoreinfo (line, true, false, true);
			if (status != SOCKET_STATUS_OK)
			// If there is another timeout: At this point we know the
			// connection is probably bad.  The socket has not been torn down
			// yet, but the read has timed out again, with no received data.
				throw imap_socket_err();
			if (line.find (tag() + "OK") != 0)
				// We may receive email notification before the server
				// receives the DONE command, in which case we would get
				// something like "XXX EXISTS" here before "OK IDLE".
				// At this point we assume this is the case, and fallout of
				// this method with the intent that the calling method can
			    // handle this.
				idleRenew = false;
		}
		else if (status != SOCKET_STATUS_OK) throw imap_socket_err();
	} while (idleRenew);

	return line;
}

/**
 * Sending the IMAP command "LOGIN" to the server.
 *
 * @exception imap_command_err
 *                     This exception is thrown when we get an unexpected
 *                     response.
 * @exception imap_dos_err
 *                     This exception is thrown when a DoS attack is suspected.
 * @exception imap_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Imap4::command_login (void) throw (imap_err)
{
	// Sending the command
	sendline ("LOGIN \"" + username() + "\" \"" + password() + "\"", false);

#ifdef DEBUG
	// Just in case someone sends me the output: password won't be displayed
	std::string line = tag() + "LOGIN \"" + username() + "\" (password) \r\n";
	g_message ("[%d] SEND(%s:%d): %s", uin(), address().c_str(), port(),
			   line.c_str());
#endif

	// Getting the acknowledgment
	waitfor_ack();
}

/**
 * Sending the IMAP command "LOGOUT" to the server. If this succeeds the
 * connection to the IMAP server is closed.
 *
 * @exception imap_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Imap4::command_logout (void) throw (imap_err)
{
	// Sending the command
	sendline ("LOGOUT");
	// Acknowledgment is read (and ignored) when closing socket
	// Closing the socket
	socket_->close ();
}

/**
 * Sending the IMAP command "SELECT" to the server. The user chosen folder on
 * the server is selected.
 *
 * @exception imap_command_err
 *                     This exception is thrown when we get an unexpected
 *                     response.
 * @exception imap_dos_err
 *                     This exception is thrown when a DoS attack is suspected.
 * @exception imap_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Imap4::command_select (void) throw (imap_err)
{
	gchar *buffer=utf8_to_imaputf7 (folder().c_str(),-1);
	if (!buffer) throw imap_command_err();

	// Send command
	sendline (std::string("SELECT \"") + buffer + "\"");
	g_free(buffer);

	// Create error message
	buffer = g_strdup_printf (_("[%d] Unable to select folder %s on host %s"),
							  uin(), folder().c_str(), address().c_str());
	if (!buffer) throw imap_command_err();
	std::string msg=std::string(buffer);
	g_free (buffer);

	// According to RFC 3501 6.3.1 there must be exactly seven lines
	// before getting the acknowledgment line.
	waitfor_ack (msg,7);

	// Check for UIDVALIDITY response code; see RFC 3501 2.3.1.1
	if (ok_response_codes_.find("UIDVALIDITY") != ok_response_codes_.end())
		uidvalidity_ = ok_response_codes_["UIDVALIDITY"];
}

/**
 * Sending the IMAP command "SEARCH NOT SEEN" and parsing the server's
 * response. The IMAP command "SEARCH NOT SEEN" is sent to the server to get
 * the message sequence numbers of those messages that have not been read yet.
 * 
 * @return             C++ vector of integers for the message sequence numbers
 *                     of unread messages.
 * @exception imap_command_err
 *                     This exception is thrown when we get an unexpected
 *                     response.
 * @exception imap_dos_err
 *                     This exception is thrown when a DoS attack is suspected.
 * @exception imap_socket_err
 *                     This exception is thrown if a network error occurs.
 */
std::set<guint> 
Imap4::command_searchnotseen (void) throw (imap_err)
{
	// Sending the command
	sendline ("SEARCH NOT SEEN");

	// Wait for "* SEARCH" untagged response
	waitfor_untaggedresponse (0, "SEARCH");
	std::stringstream ss (last_untagged_response_cont_);

	// Parse server's answer. Should be something like
	// "* SEARCH 1 2 3 4" or "* SEARCH"
	std::set<guint> buffer;
	guint n, cnt=0;
	while ((ss >> n) && (!biff_->value_bool ("use_max_mail") ||
						 (cnt++ < biff_->value_uint ("max_mail"))))
		buffer.insert (n);

	// Getting the acknowledgment
	waitfor_ack();

	return buffer;
}

/**
 * Reading and discarding input lines from the server's response for the last
 * sent command. If the response is not positive the optional error message
 * {\em msg} is printed and an imap_command_err exception is thrown.
 * Response codes for "* OK" responses are saved in Imap4::ok_response_codes_,
 * which is being reset when calling this function.
 *
 * @param     msg      Error message to be printed if we don't get a positive
 *                     response. The default is to print no message.
 * @param     num      Number of lines that are expected to be sent by the
 *                     server. This value is needed to help deciding whether
 *                     we are DoS attacked. The default value is 0.
 * @exception imap_command_err
 *                     This exception is thrown when we get an unexpected
 *                     response.
 * @exception imap_dos_err
 *                     This exception is thrown when a DoS attack is suspected.
 * @exception imap_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Imap4::waitfor_ack (std::string msg, gint num) throw (imap_err)
{
	std::string line;

	// Reset server response code map
	ok_response_codes_.clear ();

	num += 1 + biff_->value_uint ("prevdos_additional_lines");
	while ((readline (line)) && (num--))
		if (line.find (tag()) == 0)
			break;
	// Error?
	if (num<0) {
		g_warning (_("[%d] Unable to get acknowledgment from %s on port %d"),
				   uin(), address().c_str(), port());
		throw imap_dos_err();
	}

	// Negative response?
	if (line.find (tag() + "OK") != 0) {
		// Print error message
		if (msg != "")
			g_warning (msg.c_str());
		// We still have a connection to the server so we can logout
		command_logout();
		throw imap_command_err();
	}
}

/**
 * Reading and discarding input lines from the server's response until a
 * specified untagged response or a acknowledgment is read. Testing for the
 * specified untagged response succeeds for any message sequence number.
 *
 * Response codes for "* OK" responses are saved in Imap4::ok_response_codes_,
 * which is being reset when calling this function.
 *
 * @param key       Key to be tested
 * @param contbegin This is tested for being the prefix of the contents part of
 *                  the response. The default is the empty string.
 * @param num       Number of lines that are expected to be sent by the
 *                  server before the untagged response. This value is
 *                  needed to help deciding whether we are DoS attacked.
 *                  The default value is 0.
 * @return          True if the specified untagged response was sent, false if
 *                  the acknowledgment was sent.
 * @exception imap_dos_err
 *                  This exception is thrown when a DoS attack is suspected.
 * @exception imap_socket_err
 * @see             Imap4::waitfor_ack(), Imap4::waitfor_untaggedresponse()
 */
gboolean 
Imap4::waitfor_ack_untaggedresponse (std::string key, std::string contbegin,
									 gint num) throw (imap_err)
{
	std::string line;

	// Reset server response code map
	ok_response_codes_.clear ();

	// We need to set a limit to lines read (DoS attacks).
	num += 1 + biff_->value_uint ("prevdos_additional_lines");

	while (num--) {
		readline (line);
		if (test_untagged_response (key, contbegin))
			return true;
		if (line.find (tag()) == 0)
			break;
	}

	if (num < 0) {
		g_warning (_("[%d] Server doesn't send untagged \"%s\" response or acknowledgment"),
				   uin(), key.c_str());
		throw imap_dos_err();
	}

	// Negative response?
	if (line.find (tag() + "OK") != 0) {
		// We still have a connection to the server so we can logout
		command_logout();
		throw imap_command_err();
	}

	return false;
}

/**
 * Reading and discarding input lines from the server's response until a
 * specified untagged response is read. If no such line is read in time a
 * DoS attack is suspected and an imap_dos_err exception is thrown.
 *
 * Response codes for "* OK" responses are saved in Imap4::ok_response_codes_,
 * which is being reset when calling this function.
 *
 * @param msn       Message sequence number to be tested. This must be 0 if the
 *                  response shall not contain a message sequence number.
 * @param key       Key to be tested
 * @param contbegin This is tested for being the prefix of the contents part of
 *                  the response. The default is the empty string.
 * @param num       Number of lines that are expected to be sent by the
 *                  server before the untagged response. This value is
 *                  needed to help deciding whether we are DoS attacked.
 *                  The default value is 0.
 * @exception imap_dos_err
 *                  This exception is thrown when a DoS attack is suspected.
 * @exception imap_socket_err
 */
void 
Imap4::waitfor_untaggedresponse (guint msn, std::string key, 
								 std::string contbegin,gint num)throw(imap_err)
{
	std::string line;

	// Reset server response code map
	ok_response_codes_.clear ();

	// We need to set a limit to lines read (DoS attacks).
	num += 1 + biff_->value_uint ("prevdos_additional_lines");

	while (num--) {
		readline (line);
		if (test_untagged_response (msn, key, contbegin))
			return;
	}
	g_warning (_("[%d] Server doesn't send untagged \"%s\" response"), uin(),
			   key.c_str());
	throw imap_dos_err();
}

/**
 * Determine if the whole response to the "FETCH (BODYSTRUCTURE)" command has
 * been read. The server's response may be multiline. So gnubiff must know if
 * another line has to be read. This is done be counting opening and closing
 * parentheses ("(" and ")"). The level of nesting before getting to the
 * current line is given by the parameter {\em nestlevel}.
 *
 * @param  line        Current line of server's response
 * @param  nestlevel   Level of nesting before parsing response {\em line}
 * @return             Level of nesting after parsing response {\em line}
 * @exception imap_command_err
 *                     This exception is thrown when the structure of the
 *                     response {\em line} is not what we expected.
 */
guint 
Imap4::isfinished_fetchbodystructure (std::string line, guint nestlevel)
									  throw (imap_err)
{
	gint len=line.size(), pos=0;

	while (pos<len)	{
		gchar c=line[pos++];

		// String (FIXME: '"' inside of strings?)
		if (c=='"')	{
			// Read whole string
			while ((pos<len) && (line[pos++]!='"'));
			// We don't allow line breaks inside of strings. Is this a problem?
			if (pos>=len) throw imap_command_err();
			continue;
		}

		// Nested "( ... )" block begins
		if (c=='(') {
			nestlevel++;
			continue;
		}

		// Nested "( ... )" block ends
		if (c==')')	{
			nestlevel--;
			// Back at toplevel we expect to be at the end of the line
			if ((nestlevel == 0) && (pos!=len-1) && (line[pos]!='\r'))
				throw imap_command_err();
			if (nestlevel == 0) return 0;
			continue;
		}

		// All other characters are ignored
	}
	return nestlevel;
}

/** 
 * Parse the body structure of a mail.
 * This function parses the result {\em structure} of a
 * "FETCH ... (BODYSTRUCTURE)" IMAP command. It returns (via the reference
 * parameter {\em partinfo} the part of the mail body containing the first
 * "text/plain" section and information about this part. If no such section
 * exists (or in case of an error) false is returned.
 *
 * @param  structure C++ String containing the result of the IMAP command
 *                   (without "* ... FETCH (BODYSTRUCTURE (" and the trailing
 *                   ')').
 * @param  partinfo  Reference to a PartInfo structure. If true is
 *                   returned the structure will contain the information about
 *                   the selected part (part, size, encoding, charset,
 *                   mimetype)
 * @param  toplevel  Boolean (default value is true). This is true if it is the
 *                   toplevel call of this function, false if it is called
 *                   recursively.
 * @return           C++ String containing the first "text/plain" part or an
 *                   empty string
 */
gboolean 
Imap4::parse_bodystructure (std::string structure, PartInfo &partinfo,
							gboolean toplevel)
{
	gint len=structure.size(),pos=0,block=1,nestlevel=0,startpos=0;
	gboolean multipart=false;

	// Multipart? -> Parse recursively
	if (structure.at(0)=='(')
		multipart=true;

	// Length is in the 7th block:-(
	while (pos<len)
	{
		gchar c=structure.at(pos++);

		// String (FIXME: '"' inside of strings?)
		if (c=='"')
		{
			// When in multipart only the last entry is allowed to be a string
			if ((multipart) && (nestlevel==0))
				return false;
			// Get the string
			gint oldpos=pos;
			while ((pos<len) && (structure.at(pos++)!='"'));
			if ((nestlevel==0) && (!multipart))
			{
				std::string value=structure.substr(oldpos,pos-oldpos-1);
				gchar *lowercase=g_utf8_strdown(value.c_str(),-1);
				value=std::string(lowercase);
				g_free(lowercase);
				switch (block)
				{
					case 1: // MIME type
						if (value!="text")
							return false;
						partinfo.mimetype_=value;
						break;
					case 2: // MIME type
						if (value!="plain")
							return false;
						partinfo.mimetype_+="/"+value;
						break;
					case 6:	// Encoding
						partinfo.encoding_=value;
						break;
				}
			}
			continue;
		}

		// Next block
		if (c==' ')
		{
			if (nestlevel==0)
				block++;
			if ((block>7) && (!multipart))
				return false;
			while ((pos<len) && (structure.at(pos)==' '))
				pos++;
			continue;
		}

		// Nested "( ... )" block begins
		if (c=='(')
		{
			if (nestlevel==0)
				startpos=pos-1;
			nestlevel++;
			continue;
		}

		// Nested "( ... )" block ends
		if (c==')')
		{
			nestlevel--;
			if (nestlevel<0)
				return false;
			// Content of block
			std::string content=structure.substr(startpos+1,pos-startpos-2);
			// One part of a multipart message?
			if ((nestlevel==0) && (multipart))
			{
				if (!parse_bodystructure(content,partinfo,false))
					continue;
				std::stringstream ss;
				ss << block;
				if (toplevel)
					partinfo.part_=ss.str();
				else
					partinfo.part_=ss.str()+std::string(".")+partinfo.part_;
				return true;
			}
			// List of parameter/value pairs? (3rd block)
			if ((nestlevel==0) && (!multipart) && (block==3))
				if (!parse_bodystructure_parameters(content,partinfo))
					return false;
			continue;
		}

		// Alphanumerical character
		if (g_ascii_isalnum(c))
		{
			if ((multipart) && (nestlevel==0))
				return false;
			if (!multipart)
				startpos=pos-2;
			while ((pos<len) && (g_ascii_isalnum(structure.at(pos))))
				pos++;
			// Block with size information?
			if ((block==7) && (nestlevel==0) && (!multipart))
			{
				std::stringstream ss;
				ss << structure.substr(startpos,pos-startpos).c_str();
				ss >> partinfo.size_;
				partinfo.part_=std::string("1");
				return true;
			}
			continue;
		}

		// Otherwise: Error!
		return false;
	}
	// At end and no length found: Error!
	return false;
}

/** 
 * Parse the list of parameter/value pairs of a part of a mail.
 * This list is the third block of the body structure of this part. Currently
 * the only parameter we are interested in is the character set.
 * If the parameter is not in the list an empty string is returned as value for
 * this parameter.
 *
 * @param  list      C++ String containing the parameter/value list. This is a
 *                   (converted to lower case) substring of the result of the
 *                   IMAP "FETCH ... (BODYSTRUCTURE) command).
 * @param  partinfo  Reference to a PartInfo structure. If true is
 *                   returned the structure will contain the information about
 *                   the selected part (part, size, encoding, charset,
 *                   mimetype)
 * @return           Boolean indicating success or failure.
 */
gboolean 
Imap4::parse_bodystructure_parameters (std::string list, PartInfo &partinfo)
{
	gint len=list.size(),pos=0,stringcnt=1;
	std::string parameter;

	while (pos<len)
	{
		gchar c=list.at(pos++);

		// Next string
		if (c==' ')
		{
			while ((pos<len) && (list.at(pos)==' '))
				pos++;
			stringcnt++;
			continue;
		}

		// String (FIXME: '"' inside of strings?)
		if (c=='"')
		{
			gint startpos=pos;
			while ((pos<len) && (list.at(pos++)!='"'));
			std::string value=list.substr(startpos,pos-startpos-1);
			if (stringcnt%2)
			{
				// Parameter: convert to lower case
				gchar *lowercase=g_utf8_strdown(value.c_str(),-1);
				parameter=std::string(lowercase);
				g_free(lowercase);
				continue;
			}

			// Look for parameters we need
			if (parameter=="charset")
				partinfo.charset_=value;
			continue;
		}

		// Otherwise: Error!
		return false;
	}
	return true;
}

/**
 * Reset the counter for tagging imap commands.
 */
void 
Imap4::reset_tag ()
{
	tag_=std::string("");
	tagcounter_=0;
}

/**
 * Give the tag (including the following space) of the last sent IMAP command.
 *
 * @return  a C++ string with the tag
 */
std::string 
Imap4::tag ()
{
	return tag_;
}

/**
 * Save the response code from the last line sent by the server. This line
 * must contain a response code, otherwise an imap_command_err exception is
 * thrown.
 *
 * @param rc_map   Map for response codes (pairs of atoms and arguments)
 * @exception imap_command_err
 *                 This exception is thrown if {\em line} doesn't contain a
 *                 valid response code.
 */
void 
Imap4::save_response_code (std::map<std::string,std::string> &rc_map)
						   throw (imap_err)
{
	gboolean is_string = false;
	guint pos = 0, startpos = 1;

	// No response code in line
	if (last_untagged_response_cont_[0] != '[') throw imap_command_err();

	// Get end of response code
	while ((++pos) < last_untagged_response_cont_.size()) {
		if (last_untagged_response_cont_[pos] == '"') // FIXME: '"' in strings?
			is_string = !is_string;
		if ((last_untagged_response_cont_[pos] == ']') && !is_string)
			break;
	}
	if (pos == last_untagged_response_cont_.size ()) throw imap_command_err();

	// Get atom and (if available) arguments
	std::string rc=last_untagged_response_cont_.substr(startpos, pos-startpos);
	std::string atom, arg;
	pos=rc.find(" ");
	if (pos==std::string::npos)
		atom = rc;
	else {
		atom = rc.substr (0, pos);
		arg = rc.substr (pos+1);
	}

	// Save response code
	rc_map[atom] = arg;
#ifdef DEBUG
	g_message ("[%d] Saved response code to untagged status response: atom=\"%s\" arg=\"%s\"",
			   uin(), atom.c_str(), arg.c_str());
#endif
}

/**
 * Parse untagged server's response. If the given line {\em line} is an
 * untagged response it is split into the message number (if present), the
 * keyword that gives the type of the response and the contents (if present).
 * If {\em line} is no untagged response this function returns immediately.
 *
 * @param line     Response line of the server
 * @exception imap_command_err
 *                 This exception is thrown if {\em line} doesn't contain a
 *                 untagged response that is not valid.
 */
void 
Imap4::save_untagged_response (std::string &line) throw (imap_err)
{
	// Is there no untagged response?
	if (line.find("* ") != 0) {
		last_untagged_response_ = false;
		return;
	}

	// Defaults
	last_untagged_response_ = true;
	last_untagged_response_msn_ = 0;
	last_untagged_response_cont_ = std::string("");
	last_untagged_response_key_ = std::string("");

	guint pos = 2;
	// Handling the message sequence number
	if (g_ascii_isdigit (line[2])) {
		while (g_ascii_isdigit (line[++pos]));// terminates because '\r' at end
		pos++;
		std::stringstream ss (line.substr (2, pos-3));
		ss >> last_untagged_response_msn_;
	}

	// Find the separating space between key and contents (if it exists)
	guint pos_sep = line.find (" ", pos);

	if (pos_sep == std::string::npos)
		last_untagged_response_key_ = line.substr (pos, line.size()-pos-1);
	else {
		if (pos == pos_sep) throw imap_command_err();
		last_untagged_response_key_ = line.substr (pos, pos_sep-pos);
		last_untagged_response_cont_ = line.substr (pos_sep+1,
												   line.size()-pos_sep);
	}
}

/**
 * Test if the last line sent by the server was the specified untagged
 * response.
 *
 * @param msn       Message sequence number to be tested. This must be 0 if the
 *                  response shall not contain a message sequence number.
 * @param key       Key to be tested
 * @param contbegin This is tested for being the prefix of the contents part of
 *                  the response. The default is the empty string.
 * return           True if the last line sent by the server was a untagged
 *                  response with the given attributes.
 */
gboolean 
Imap4::test_untagged_response (guint msn, std::string key,
							   std::string contbegin)
{
	return (last_untagged_response_
			&& (msn == last_untagged_response_msn_)
			&& (key == last_untagged_response_key_)
			&& (last_untagged_response_cont_.find (contbegin) == 0));
}

/**
 * Test if the last line sent by the server was the specified untagged
 * response for any message sequence number. The value of this message
 * sequence number is saved in Imap4::last_untagged_response_msn_.
 *
 * @param key       Key to be tested
 * @param contbegin This is tested for being the prefix of the contents part of
 *                  the response. The default is the empty string.
 * return           True if the last line sent by the server was a untagged
 *                  response with the given attributes.
 */
gboolean 
Imap4::test_untagged_response (std::string key,	std::string contbegin)
{
	return (last_untagged_response_
			&& (0 != last_untagged_response_msn_)
			&& (key == last_untagged_response_key_)
			&& (last_untagged_response_cont_.find (contbegin) == 0));
}

/**
 * Send an IMAP command.
 * The given {\em command} is prefixed with a unique identifier (obtainable
 * via the Imap::tag() function) and postfixed with "\r\n" and then written to
 * the socket of the mailbox.
 *
 * If {\em check} is true the return value of the call to Socket::write() is
 * checked and an imap_socket_err exception is thrown if it was not successful.
 * So this function always returns SOCKET_STATUS_OK if {\em check} is true,
 * otherwise (if {\em check} is false) error handling is left to the caller of
 * this function.
 *
 * @param command  IMAP command line to be sent
 * @param print    Shall the sent command be printed in debug mode?
 *                 The default is true.
 * @param check    Shall the return value of the Socket::write() command be
 *                 checked? The default is true.
 * @return         Return value of the Socket::write() command, this is always
 *                 SOCKET_STATUS_OK if {\em check} is true.
 * @exception imap_command_err
 *                 This exception is thrown if we can't create the string to be
 *                 sent to the server.
 * @exception imap_socket_err
 *                 This exception is thrown if a network error occurs.
 */
gint 
Imap4::sendline (const std::string command, gboolean print, gboolean check)
				 throw (imap_err)
{
	// Create new tag
	tagcounter_++;
	gchar *buffer=g_strdup_printf("A%05d ",tagcounter_);
	if (buffer==NULL) throw imap_command_err();
	tag_=std::string(buffer);
	g_free(buffer);

	// Write line
	gint status=socket_->write (tag_ + command + "\r\n", print);
	if ((status!=SOCKET_STATUS_OK) && check) throw imap_socket_err();
	return status;
}

/**
 * Send an IMAP command. The command line will be created by concatenating
 * {\em command}, {\em msn} and {\em arg}.
 *
 * @param command  IMAP command to be sent
 * @param msn      Message sequence number argument to the command
 * @param arg      Other arguments to the command
 * @param print    Shall the sent command be printed in debug mode?
 *                 The default is true.
 * @param check    Shall the return value of the Socket::write() command be
 *                 checked? The default is true.
 * @return         Return value of the Socket::write() command, this is always
 *                 SOCKET_STATUS_OK if {\em check} is true.
 * @exception imap_command_err
 *                 This exception is thrown if we can't create the string to be
 *                 sent to the server.
 * @exception imap_socket_err
 *                 This exception is thrown if a network error occurs.
 * @see            The description of the method Imap4::sendline() contains a
 *                 more extensive description for the parameter {\em check}.
 */
gint 
Imap4::sendline (const std::string command, guint msn,
				 const std::string arg, gboolean print, gboolean check)
				 throw (imap_err)
{
	std::stringstream ss;
	ss << msn;
	return sendline (command + " " + ss.str() + " " + arg, print, check);
}

/**
 * Read one line from the server. If {\em check} is true the return value of
 * the call to Socket::read() is checked and an imap_socket_err exception is
 * thrown if it was not successful. So this function always returns
 * SOCKET_STATUS_OK if {\em check} is true, otherwise (if {\em check} is
 * false) error handling is left to the caller of this function.
 *
 * If {\em checkline} is true then the read line is checked for being an
 * untagged response. If it is an untagged response the line is parsed and its
 * values are saved for later use.
 *
 * If {\em checkline} is true then the read line is checked for being an
 * untagged status response. If an error response is found
 * ("* BYE" or "* BAD") an error message is printed and an imap_command_err
 * exception is thrown. If a warning response ("* NO") is found the warning is
 * printed.
 *
 * If {\em checkline} is true server response codes for "* OK" server status
 * responses are saved in Imap4::ok_response_codes_. No response codes are
 * currently saved for other status responses than "* OK".
 *
 * Remark: If the mailbox is not checking for new mail and we get a "* BYE"
 * message (when idling for example) an imap_command_err exception is thrown
 * but mailbox status will not be set to MAILBOX_ERROR.
 *
 * Remark: The parameter {\em checkline} must be false if reading the response
 * to the "LOGOUT" command because in this situation an untagged "* BYE"
 * response doesn't indicate an error.
 *
 * @param line      String that contains the read line if the call was
 *                  successful (i.e. the return value is SOCKET_STATUS_OK),
 *                  the value is undetermined otherwise
 * @param print     Shall the read line be printed in debug mode?
 *                  The default is true.
 * @param check     Shall the return value of the Socket::read() command be
 *                  checked? The default is true.
 * @param checkline Shall {\em line} be checked for an untagged negative
 *                  response? The default is true.
 * @return          Return value of the Socket::read() command, this is always
 *                  SOCKET_STATUS_OK if {\em check} is true.
 * @exception imap_command_err
 *                  This exception is thrown if {\em line} contains a negative
 *                  untagged response and {\em checkline} is true.
 * @exception imap_socket_err
 *                  This exception is thrown if a network error occurs.
 */
gint 
Imap4::readline (std::string &line, gboolean print, gboolean check,
				 gboolean checkline) throw (imap_err)
{
	// Read line
	gint status=socket_->read(line, print, check);
	if (check && (status != SOCKET_STATUS_OK)) throw imap_socket_err();

	// Check for an untagged negative response
	if (!checkline)
		return status;
	// Save untagged response
	save_untagged_response (line);
	if (!last_untagged_response_)
		return status;
	// Parse specific untagged responses
	if (test_untagged_response (0, "OK", "[")) // see RFC 3501 7.1
		save_response_code(ok_response_codes_);
	else if (test_untagged_response (0, "BYE")) { // see RFC 3501 7.1.5
		g_warning (_("[%d] Server closes connection immediately:%s"),
				   uin(), line.substr(5,line.size()-5).c_str());
		throw imap_command_err (Mailbox::status() == MAILBOX_CHECK);
	}
	else if (test_untagged_response (0, "BAD")) { // see RFC 3501 7.1.3
		g_warning (_("[%d] Internal server failure or unknown error:%s"),
				   uin(), line.substr(5,line.size()-5).c_str());
		throw imap_command_err();
	}
	else if (test_untagged_response (0, "NO")) { // see RFC 3501 7.1.2
		g_warning (_("[%d] Warning from server:%s"), uin(),
				   line.substr(4,line.size()-4).c_str());
	}
	return status;
}

/**
 * Read one line from the server and ignore warning and information message
 * lines.
 *
 * @param line      String that contains the read line if the call was
 *                  successful (i.e. the return value is SOCKET_STATUS_OK),
 *                  the value is undetermined otherwise
 * @param print     Shall the read lines be printed in debug mode?
 *                  The default is true.
 * @param check     Shall the return value of the Socket::read() command be
 *                  checked? The default is true.
 * @param checkline Shall {\em line} be checked for an untagged negative
 *                  response? The default is true.
 * @return          Return value of the Socket::read() command, this is always
 *                  SOCKET_STATUS_OK if {\em check} is true.
 * @exception imap_command_err
 *                  This exception is thrown if {\em line} contains a negative
 *                  untagged response and {\em checkline} is true.
 * @exception imap_dos_err
 *                  This exception is thrown when a DoS attack is suspected.
 * @exception imap_socket_err
 *                  This exception is thrown if a network error occurs.
 * @see             The description of the method Imap4::readline() contains a
 *                  more extensive description for the parameters {\em check}
 *                  and {\em checkline} and some information regarding
 *                  exception handling.
 */
gint 
Imap4::readline_ignoreinfo (std::string &line, gboolean print, gboolean check,
							gboolean checkline) throw (imap_err)
{
	gint cnt = 1 + biff_->value_uint ("prevdos_ignore_info"), status;

	do {
		status=readline (line, print, check, checkline);
		// Check for information or warning message
		if (!last_untagged_response_)
			break;
		if (last_untagged_response_key_ == "OK")
			continue;
		if (last_untagged_response_key_ != "NO")
			break;
	} while ((status==SOCKET_STATUS_OK) && (cnt--));
	if (cnt < 0) throw imap_dos_err();

	return status;
}
