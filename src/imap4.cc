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
Imap4::Imap4 (Biff *biff) : Mailbox (biff)
{
	protocol_ = PROTOCOL_IMAP4;
	socket_   = new Socket (this);
	idleable_ = false;
	idled_    = false;
}

Imap4::Imap4 (const Mailbox &other) : Mailbox (other)
{
	protocol_ = PROTOCOL_IMAP4;
	socket_   = new Socket (this);
	idleable_ = false;
	idled_    = false;
}

Imap4::~Imap4 (void)
{
	delete socket_;
}

// ========================================================================
//  main
// ========================================================================	
void
Imap4::threaded_start (guint delay)
{
	// Is there already a timeout?
	if (timetag_)
		return;

	// Are we in idle state?
	if (idled_)
		return;

	// Do we want to start using given delay?
	if (delay) {
		timetag_ = g_timeout_add (delay*1000, start_delayed_entry_point, this);
#if DEBUG
		g_message ("[%d] Start fetch in %d second(s)", uin_, delay);
#endif
	}
	//  or internal delay?
	else {
		timetag_ = g_timeout_add (delay_*1000, start_delayed_entry_point, this);
#if DEBUG
		g_message ("[%d] Start fetch in %d second(s)", uin_, delay_);
#endif
	}
}

void 
Imap4::start (void)
{
	if (!g_mutex_trylock (monitor_mutex_))
		return;

	try	{
		fetch ();
		send ("LOGOUT");
	}
	catch (imap_err& err) {
		// Catch all errors that are un-recoverable and result in
		// closing the connection, and resetting the mailbox status.
#if DEBUG
		g_message("[%d] Imap exception: %s", uin_, err.what());
#endif
		status_ = MAILBOX_ERROR;
		unread_.clear();
		seen_.clear();
		saved_.clear();
	}

	idled_ = false;
	socket_->close ();
	update_applet();

	g_mutex_unlock (monitor_mutex_);

	threaded_start (delay_);
}

/**
 * Connect to the mailbox, get unread mails and update mailbox status.
 * If the password for the mailbox isn't already known, it is obtained (if
 * possible). If the mailbox supports the "IDLE" command this function starts
 * idling once the mailbox status is known.
 *
 * @exception imap_command_err
 *                     If we get an unexpected server's response
 * @exception imap_dos_err
 *                     If an DoS attack is suspected.
 * @exception imap_nologin_err
 *                     The server doesn't want us to login or the user doesn't
 *                     provide a password.
 * @exception imap_socket_err
 *                     If a network error occurs
 */
void 
Imap4::fetch (void)
{
	// Is there a password? Can we obtain it?
	if (!biff_->password(this)) throw imap_nologin_err();

	// Connection and authentification
	if (!connect ()) throw imap_socket_err();

	// Set the mailbox status and get mails (if there is new mail)
	fetch_mails();

	// Start idling (if possible)
	if (idleable_) {
		idled_ = true;
		idle();
	}
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

	// If we have reported the new mail, then set the status to old
	if (status_ == MAILBOX_NEW)
		status_ = MAILBOX_OLD;
}

gint 
Imap4::connect (void)
{
	std::string line;

	// Check standard port
	if (!use_other_port_)
		if (authentication_ == AUTH_USER_PASS)
			port_ = 143;
		else
			port_ = 993;

#ifdef DEBUG
	g_message ("[%d] Trying to connect to %s on port %d", uin_, address_.c_str(), port_);
#endif

	// connection
	if (authentication_ == AUTH_AUTODETECT) {
		guint port = port_;
		if (!use_other_port_)
			port = 993;
		if (!socket_->open (address_, port, AUTH_SSL)) {
			if (!use_other_port_)
				port = 143;
			if (!socket_->open (address_, port, AUTH_USER_PASS)) {
				status_ = MAILBOX_ERROR;
				return 0;
			}
			else {
				port_ = port;
				authentication_ = AUTH_USER_PASS;
				socket_->close();
			}
		}
		else {
			port_ = port;
			authentication_ = AUTH_SSL;
			socket_->close();
		}
	}

	if (!socket_->open (address_, port_, authentication_, certificate_, 3)) {
		status_ = MAILBOX_ERROR;
		return 0;
	}

	// Set reads from the socket to time out.	We do this primarily for
	// the IDLE state.	However, this also prevents reads in general
	// from blocking forever on connections that have gone bad.	 We
	// don't let the timeout period be less then 60 seconds.
	socket_->set_read_timeout(delay_ < 60 ? 60 : delay_);

#ifdef DEBUG
	g_message ("[%d] Connected to %s on port %d", uin_, address_.c_str(), port_);
#endif
	if (!(socket_->read (line, true))) return 0;

	// Resetting the tag counter
	reset_tag();

	// CAPABILITY
	command_capability();

	// LOGIN
	command_login();

	// SELECT
	command_select();

	return 1;
}

/**
 * Get the first lines of the unread mails and update the status of the
 * mailbox.
 *
 * @exception imap_command_err
 *                     If we get an unexpected server's response
 * @exception imap_dos_err
 *                     If an DoS attack is suspected.
 * @exception imap_socket_err
 *                     If a network error occurs
 */
void 
Imap4::fetch_mails (void)
{
	// Status will be restored in the end if no problem occured
	status_ = MAILBOX_CHECK;

	// SEARCH NOT SEEN
	std::vector<int> buffer=command_searchnotseen();
	
	// Get new mails one by one
	new_unread_.clear();
	new_seen_.clear();
	for (guint i=0; (i<buffer.size()) && (new_unread_.size() < (unsigned int)(biff_->max_mail_)); i++) {

		// FETCH header information
		std::vector<std::string> mail=command_fetchheader(buffer[i]);

		// FETCH BODYSTRUCTURE
		PartInfo partinfo=command_fetchbodystructure(buffer[i]);

		// FETCH BODY
		command_fetchbody (buffer[i], partinfo, mail);

		// Decode and parse mail
		if (partinfo.part_!="")
			decode_body (mail, partinfo.encoding_);
		parse (mail, MAIL_UNREAD);
	}
	
	// Set mailbox status
	if (buffer.empty())
		status_ = MAILBOX_EMPTY;
	else if (contains_new<header>(new_unread_, unread_))
		status_ = MAILBOX_NEW;
	else
		status_ = MAILBOX_OLD;

	unread_ = new_unread_;
	seen_ = new_seen_;
}

/**
 * Cleanup, then close the connection to the IMAP server.
 */
void 
Imap4::close (void)
{
	// Closing connection
	send ("LOGOUT");	
	socket_->close ();
}

/**
 * Begin the IMAP idle mode.  This method will not return until
 * either we receive IMAP notifications (new mail...), or the server
 * terminates for some reason.
 *
 * @exception imap_command_err
 *                     If we get an unexpected server's response
 * @exception imap_dos_err
 *                     If an DoS attack is suspected.
 * @exception imap_socket_err
 *                     If a network error occurs
 */
void 
Imap4::idle (void) throw (imap_err)
{

	// currently we will never exit this loop unless an error occurs,
	// Probably due to the loss of a connection.	Basically our loop is:
	// (update applet)->(wait in idle for mail change)->(Get Mail headers)
	while (true)
	{
		// When in idle state, we won't exit this thread function
		// so we have to, update applet in the meantime
		update_applet();
		
		if (timetag_)
			g_source_remove (timetag_);
		timetag_ = 0;
		
		std::string line = idle_renew_loop();
		
		// Did we loose the lock?
		if (line.find ("* BYE") == 0) throw imap_command_err();
		
		if (!socket_->write (std::string("DONE\r\n"))) throw imap_socket_err();
		
		// Either we got a OK or a BYE
		gint cnt=preventDoS_additionalLines_;
		do {
			if (!socket_->read (line)) throw imap_socket_err();
			// Did we lost lock?
			if (line.find ("* BYE") == 0) throw imap_command_err();
		} while ((line.find (tag()+"OK") != 0) && (cnt--));
		if (!cnt)
			throw imap_dos_err();

		fetch_mails();
	}
}

/**
 * idle_loop enters into the idle mode by issueing the imap "IDLE"
 * command, then waits for notifications from the IMAP server.
 * With inactivity the socket read will timeout periodically waiting for server
 * notifications.  When the timeout occurs we simply issue the IMAP
 * "DONE" command then re-enter the idle mode again.  The timeout
 * occurs every {\em delay_} + 1 minute time.  We perform this timeout
 * operation so that we periodically test the connection to make sure it
 * is still valid, and to also keep the connection from being closed by
 * keeping the connection active.
 * 
 * @return         Returns the last line received from the IMAP server.
 * @exception imap_socket_err
 *                 If a network error occurs
 */
std::string 
Imap4::idle_renew_loop() throw (imap_err)
{
	gboolean idleRenew = false;	 // If we should renew the IDLE again.
	std::string line;
	do {
		idleRenew = false;

		// IDLE
		if (!send (std::string("IDLE"))) throw imap_socket_err();
		
		// Read acknowledgement
		if (!socket_->read (line)) throw imap_socket_err();
		
		// Wait for new mail and block thread at this point
		gint status = socket_->read (line);
		if (status == SOCKET_TIMEOUT) {
			// We timed out, so we want to loop, and issue IDLE again.
			idleRenew = true;

			if (!socket_->write (std::string("DONE\r\n")))
				throw imap_socket_err();

			status = socket_->read (line);
			if (line == "") {
				// At this point we know the connection is probably bad.  The
				// socket has not been torn down yet, but the read has timed
				// out again, with no received data.
				throw imap_socket_err();
			}
			if (line.find (tag() + "OK") != 0) {
				// We may receive email notification before the server
				// receives the DONE command, in which case we would get
				// something like "XXX EXISTS" here before "OK IDLE".
				// At this point we assume this is the case, and fallout
				// of the idle_renew_loop method with the intent that the
				// calling method can handle this.
				idleRenew = false;
			}
		}
		else if (status != SOCKET_STATUS_OK)
			throw imap_socket_err();
	} while (idleRenew);

	return line;
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
 * @exception imap_command_err
 *                     If we get an unexpected server's response
 * @exception imap_dos_err
 *                     If an DoS attack is suspected.
 * @exception imap_socket_err
 *                     If a network error occurs
 */
void 
Imap4::command_capability (void) throw (imap_err)
{
	std::string line;

	// Sending the command
	if (!send("CAPABILITY")) throw imap_socket_err();

	// Getting server's response
	if (!(socket_->read (line))) throw imap_socket_err();
	if (line.find ("* CAPABILITY") != 0) throw imap_command_err();

	// Getting the acknowledgment
	command_waitforack();

	// Remark: We have a space-separated listing. In order to not match
	// substrings we have to include the spaces when comparing. To match the
	// last entry we have to convert '\n' to ' '
	line[line.size()-1]=' ';

	// Looking for supported capabilities
	idleable_=(line.find (" IDLE ") != std::string::npos);

	if (line.find (" LOGINDISABLED ") != std::string::npos) {
		send ("LOGOUT");
		throw imap_nologin_err();
	}
}

/**
 * Obtain the first lines of the body of the mail with sequence number
 * {\em msn}.
 *
 * @param     msn      Unsigned integer for the message sequence number of the
 *                     mail
 * @param     partinfo Partinfo structure with information of the relevant
 *                     part of the mail as returned by
 *                     Imap4::command_fetchbodystructure().
 * @param     mail     C++ vector of C++ strings containing the header lines of
 *                     the mail (inclusive the separating empty line).
 * @exception imap_command_err
 *                     If we get an unexpected server's response
 * @exception imap_dos_err
 *                     If an DoS attack is suspected.
 * @exception imap_socket_err
 *                     If a network error occurs
 */
void 
Imap4::command_fetchbody (guint msn, class PartInfo &partinfo,
						  std::vector<std::string> &mail) throw (imap_err)
{
	std::string line;

	// Message sequence number
	std::stringstream ss;
	ss << msn;

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
	// so it is sufficient to get at most 1000*bodyLinesToBeRead_
	// bytes.
	gint textsize=partinfo.size_;
	if (textsize>1000*bodyLinesToBeRead_)
		textsize=1000*bodyLinesToBeRead_;
	std::stringstream textsizestr;
	textsizestr << textsize;

	// Send command
	line = "FETCH " + ss.str() + " (BODY.PEEK[" + partinfo.part_ + "]<0.";
	line+= textsizestr.str() + ">)";
	if (!send(line)) throw imap_socket_err();
			
	// Response should be: "* s FETCH ..." (see RFC 3501 7.4.2)
	gint cnt=1+preventDoS_additionalLines_;
	while (((socket_->read(line) > 0)) && (cnt--))
		if (line.find ("* " + ss.str() + " FETCH") == 0)
			break;
	if (!socket_->status()) throw imap_socket_err();
	if (cnt<0) throw imap_dos_err();
			
#ifdef DEBUG
	g_print ("** Message: [%d] RECV(%s:%d): (message) ", uin_,
			 address_.c_str(), port_);
#endif
	// Read text
	gint lineno=0, bytes=textsize+3; // ")\r\n" at end of mail
	while ((bytes>0) && ((socket_->read(line, false) > 0))) {
		bytes-=line.size()+1; // don't forget to count '\n'!
		if ((line.size() > 0) && (lineno++<bodyLinesToBeRead_)) {
			mail.push_back (line.substr(0, line.size()-1));
#ifdef DEBUG
			g_print ("+");
#endif
		}
	}
#ifdef DEBUG
		g_print ("\n");
#endif
	if (!socket_->status()) throw imap_socket_err();
	if (bytes<0) throw imap_dos_err();
	// Remove ")\r" from last line ('\n' was removed before)
	mail.pop_back();
	if ((line.size()>1) && (line[line.size()-2]==')'))
		mail.push_back (line.substr(0, line.size()-2));
	else
		throw imap_command_err();

	// Getting the acknowledgment
	command_waitforack();
}

/**
 * Decide which part from the mail with sequence number {\em msn} we are
 * interested in. This is done by sending IMAP command
 * "FETCH {\em msn} (BODYSTRUCTURE)" to the server and parsing the server's
 * response.
 *
 * @param     msn      Unsigned integer for the message sequence number of the
 *                     mail
 * @return             Partinfo structure with information of the relevant
 *                     part of the mail.
 * @exception imap_command_err
 *                     If we get an unexpected server's response
 * @exception imap_dos_err
 *                     If an DoS attack is suspected.
 * @exception imap_socket_err
 *                     If a network error occurs
 */
class PartInfo 
Imap4::command_fetchbodystructure (guint msn) throw (imap_err)
{
	std::string line;

	// Message sequence number
	std::stringstream ss;
	ss << msn;

	// Send command
	if (!send("FETCH " +ss.str()+ " (BODYSTRUCTURE)")) throw imap_socket_err();

	// Response should be: "* s FETCH (BODYSTRUC..." (see RFC 3501 7.4.2)
	gint cnt=1+preventDoS_additionalLines_;
	while (((socket_->read(line) > 0)) && (cnt--))
		if (line.find ("* " + ss.str() + " FETCH (BODYSTRUCTURE (") == 0)
			break;
	if (!socket_->status()) throw imap_socket_err();
	if (cnt<0) throw imap_dos_err();
	if (line.substr(line.size()-2) != ")\r") throw imap_command_err();

	// Remove first and last part
	line=line.substr(25+ss.str().size(),line.size()-28-ss.str().size());

	// Get Part of Mail that contains "text/plain" (if any exists) and
	// size of this text, encoding, charset
	PartInfo partinfo;
	parse_bodystructure(line,partinfo);
#ifdef DEBUG
	g_print("** Part %s size=%d, encoding=%s, charset=%s\n",
			partinfo.part_.c_str(), partinfo.size_, partinfo.encoding_.c_str(),
			partinfo.charset_.c_str());
#endif

	// Getting the acknowledgment
	command_waitforack();

	return partinfo;
}

/**
 * Obtain some header information from the mail with sequence number {\em msn}.
 * The IMAP command "FETCH" is sent to the server in order to obtain the From,
 * Date and Subject of the mail. The last line of the returned header lines
 * should be empty.
 * 
 * @param     msn      Unsigned integer for the message sequence number of the
 *                     mail
 * @return             C++ vector of C++ strings containing the header lines
 * @exception imap_command_err
 *                     If we get an unexpected server's response
 * @exception imap_dos_err
 *                     If an DoS attack is suspected.
 * @exception imap_socket_err
 *                     If a network error occurs
 */
std::vector<std::string> 
Imap4::command_fetchheader (guint msn) throw (imap_err)
{	
	// Start with an empty mail
	std::vector<std::string> mail;
	mail.clear();

	// Message sequence number
	std::stringstream ss;
	ss << msn;
		
	// Send command
	std::string line;
	line="FETCH "+ss.str()+" (BODY.PEEK[HEADER.FIELDS (DATE FROM SUBJECT)])";
	if (!send(line)) throw imap_socket_err();
		
	// Response should be: "* s FETCH ..." (see RFC 3501 7.4.2)
	gint cnt=1+preventDoS_additionalLines_;
	while (((socket_->read(line) > 0)) && (cnt--))
		if (line.find ("* "+ss.str()+" FETCH") == 0)
			break;
	if (!socket_->status()) throw imap_socket_err();
	if (cnt<0) throw imap_dos_err();
		
	// Date, From, Subject and an empty line
#ifdef DEBUG
	g_print ("** Message: [%d] RECV(%s:%d): (message) ", uin_,
			 address_.c_str(), port_);
#endif
	cnt=5+preventDoS_additionalLines_;
	while (((socket_->read(line, false) > 0)) && (cnt--)) {
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
	if (!socket_->status()) throw imap_socket_err();
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
 * Sending the IMAP command "LOGIN" to the server.
 *
 * @exception imap_command_err
 *                     If we get an unexpected server's response
 * @exception imap_dos_err
 *                     If an DoS attack is suspected.
 * @exception imap_socket_err
 *                     If a network error occurs
 */
void 
Imap4::command_login (void) throw (imap_err)
{
	std::string line;

	// Sending the command
	line = "LOGIN \"" + username_ + "\" \"" + password_ + "\"";
	if (!send (line, false)) throw imap_socket_err();

#ifdef DEBUG
	// Just in case someone sends me the output: password won't be displayed
	line = tag() + "LOGIN \"" + username_ + "\" (password) \r\n";
	g_message ("[%d] SEND(%s:%d): %s", uin_, address_.c_str(), port_,
			   line.c_str());
#endif

	// Getting the acknowledgment
	command_waitforack();
}

/**
 * Sending the IMAP command "SELECT" to the server. The user chosen folder on
 * the server is selected.
 *
 * @exception imap_command_err
 *                     If we get an unexpected server's response
 * @exception imap_dos_err
 *                     If an DoS attack is suspected.
 * @exception imap_socket_err
 *                     If a network error occurs
 */
void 
Imap4::command_select (void) throw (imap_err)
{
	gboolean sendok=false;
	gchar *folder_imaputf7=utf8_to_imaputf7(folder_.c_str(),-1);

	// Send command
	if (folder_imaputf7)
	{
		sendok=send(std::string("SELECT \"") + folder_imaputf7 + "\"");
		g_free(folder_imaputf7);
	}

	// Error handling
	if ((!sendok) || (!folder_imaputf7))
		g_warning (_("[%d] Unable to select folder %s on host %s"),
				   uin_, folder_.c_str(), address_.c_str());
	if (!folder_imaputf7) throw imap_command_err();
	if (!sendok) throw imap_socket_err();

	// According to RFC 3501 6.3.1 there must be exactly seven lines
	// before getting the acknowledgment line.
	command_waitforack(7);
}

/**
 * Sending the IMAP command "SEARCH NOT SEEN" and parsing the server's
 * response. The IMAP command "SEARCH NOT SEEN" is sent to the server to get
 * the message sequence numbers of those messages that have not been read yet.
 * 
 * @return             C++ vector of integers for the message sequence numbers
 *                     of unread messages.
 * @exception imap_command_err
 *                     If we get an unexpected server's response
 * @exception imap_dos_err
 *                     If an DoS attack is suspected.
 * @exception imap_socket_err
 *                     If a network error occurs
 */
std::vector<int> 
Imap4::command_searchnotseen (void) throw (imap_err)
{
	std::string line;

	// Sending the command
	if (!send("SEARCH NOT SEEN")) throw imap_socket_err();

	// We need to set a limit to lines read (DoS Attacks).
	// Expected response "* SEARCH ..." should be in the next line.
	gint cnt=1+preventDoS_additionalLines_;
	while (((socket_->read(line) > 0)) && (cnt--))
		if (line.find ("* SEARCH") == 0)
			break;
	if (!socket_->status()) throw imap_socket_err();
	if (cnt<0) throw imap_dos_err();

	// Parse server's answer. Should be something like
	// "* SEARCH 1 2 3 4" or "* SEARCH"
	// (9 is size of "* SEARCH ")
	std::vector<int> buffer;
	buffer.clear();
	if (line.size() > 9) {
		line = line.substr (9);
		int n = 0;
		for (guint i=0; i<line.size(); i++) {
			if (line[i] >= '0' && line[i] <= '9')
				n = n*10 + int(line[i]-'0');
			else {
				buffer.push_back (n);
				n = 0;
			}
		}
	}

	// Getting the acknowledgment
	command_waitforack();

	return buffer;
}

/**
 * Reading and discarding input lines from the server's response for the last
 * sent command. If the response is not positive an {\em imap_command_err}
 * exception is thrown.
 * 
 * @param     cnt      Number of lines that are expected to be sent by the
 *                     server. This value is needed to help deciding whether
 *                     we are DoS attacked. The default value is 0.
 * @exception imap_command_err
 *                     If we get an unexpected server's response
 * @exception imap_dos_err
 *                     If an DoS attack is suspected.
 * @exception imap_socket_err
 *                     If a network error occurs
 */
void 
Imap4::command_waitforack (gint cnt) throw (imap_err)
{
	std::string line;

	cnt+=1+preventDoS_additionalLines_;
	while ((socket_->read (line) > 0) && (cnt--))
		if (line.find (tag()) == 0)
			break;
	if (!socket_->status()) throw imap_socket_err();
	if (cnt<0) throw imap_dos_err();

	// Print error message and throw exception if response is not positive
	if (line.find (tag() + "OK") != 0) {
		g_warning (_("[%d] Unable to get acknowledgment from %s on port %d"),
				   uin_, address_.c_str(), port_);
		throw imap_command_err();
	}
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
Imap4::reset_tag()
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
Imap4::tag()
{
	return tag_;
}

/**
 * Send an IMAP command.
 * The given {\em command} is prefixed with a unique identifier (obtainable
 * via the tag() function) and postfixed with "\r\n" and then written to the
 * socket of the mailbox.
 *
 * @param command  IMAP command as a C++ string
 * @param debug    boolean that says if the command should be printed in debug
 *                 mode (default is true)
 * @return         return value of the socket write command
 */
gint 
Imap4::send(std::string command, gboolean debug)
{
	// Create new tag
	tagcounter_++;
	gint len=g_snprintf(NULL,0,"A%05d ",tagcounter_)+1;
	gchar *buffer=g_strnfill(len,'\0');
	if (buffer==NULL)
		return 0;
	if (g_snprintf(buffer,len,"A%05d ",tagcounter_)!=len-1)
		return 0;
	tag_=std::string(buffer);
	g_free(buffer);
	// Write line
	return socket_->write (tag_ + command + "\r\n", debug);
}
