// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2005 Nicolas Rougier
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

#include <algorithm>
#include <sstream>
#include <sys/stat.h>
#include <utime.h>

#include "ui-applet.h"
#include "ui-popup.h"
#include "pop.h"
#include "socket.h"
#include "nls.h"

// ========================================================================
//  base
// ========================================================================	
/**
 * Constructor. The mailbox for one of the IMAP protocols is created from
 * scratch.
 *
 * @param biff Pointer to the instance of Gnubiff.
 */
Pop::Pop (Biff *biff) : Mailbox (biff)
{
	socket_ = new Socket (this);
}

/**
 * Constructor. The mailbox for one of the POP protocols is created by taking
 * the attributes of the existing mailbox {\em other}.
 *
 * @param other Mailbox from which the attributes are taken.
 */
Pop::Pop (const Mailbox &other) : Mailbox (other)
{
	socket_ = new Socket (this);
}

/// Destructor
Pop::~Pop (void)
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
Pop::threaded_start (guint delay)
{
	// If no delay is given use internal delay
	if (!delay)
		delay=Mailbox::delay();

	Mailbox::threaded_start (delay);
}

/**
 * Method to be called by a new thread for monitoring the mailbox. The status
 * of the mailbox will be updated, new mails fetched and idle state entered
 * (if the server does allow this). Before exiting creating of a new thread
 * for monitoring is noted down.
 *
 * Remark: In this function all exceptions are catched that are thrown when
 * sending POP commands or receiving response from the server.
 */
void 
Pop::start (void) throw (pop_err)
{
	if (!g_mutex_trylock (monitor_mutex_))
		return;

	try {
		start_checking ();
	}
	catch (pop_err& err) {
		// Catch all errors that are un-recoverable and result in
		// closing the connection, and resetting the mailbox status.
#if DEBUG
		g_warning ("[%d] Pop exception: %s", uin(), err.what());
#endif
		status (MAILBOX_ERROR);
		unread_.clear ();
		seen_.clear ();
		socket_->close ();
	}

	if (!GTK_WIDGET_VISIBLE (biff_->popup()->get())) {
		gdk_threads_enter();
		biff_->applet()->update();
		gdk_threads_leave();
	}

	g_mutex_unlock (monitor_mutex_);

	threaded_start (delay());
}

/**
 * Connect to the mailbox, get unread mails and update mailbox status.
 * If the password for the mailbox isn't already known, it is obtained (if
 * possible). When leaving this function gnubiff will logout from the server.
 *
 * @exception pop_command_err
 *                     This exception is thrown when we get an unexpected
 *                     response.
 * @exception pop_dos_err
 *                     This exception is thrown when a DoS attack is suspected.
 * @exception pop_nologin_err
 *                     The server doesn't want us to login or the user doesn't
 *                     provide a password.
 * @exception pop_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Pop::fetch (void) throw (pop_err)
{
	// Is there a password? Can we obtain it?
	if (!biff_->password(this)) {
		g_warning (_("[%d] Empty password"), uin());
		throw pop_nologin_err();
	}

	// Connection and authentification
	connect();

	fetch_mails ();

	// QUIT
	command_quit();
}

/**
 * Get the first lines of the unread mails and update the status of the
 * mailbox.
 *
 * @exception pop_command_err
 *                     This exception is thrown when we get an unexpected
 *                     response.
 * @exception pop_dos_err
 *                     This exception is thrown when a DoS attack is suspected.
 * @exception pop_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Pop::fetch_mails (gboolean statusonly) throw (pop_err)
{
	std::map<guint,std::string> msg_uid;

	// STAT
	guint total = command_stat ();

	// We want to retrieve a maximum of _max_collected_mail uidl
	// so we have to check the total number and find corresponding
	// starting index (start).
	guint start = 1, num = biff_->value_uint ("max_mail");
	if ((biff_->value_bool ("use_max_mail")) && (total > num))
		start = 1 + total - num;
	else
		num = total;

	// UIDL
	if (num == total)
		command_uidl (total, msg_uid);

	// Fetch mails
	std::vector<std::string> mail;
	std::string uid;
	for (guint i=0; i< num; i++) {
		// UIDL
		if (msg_uid.empty())
			uid = command_uidl (i+start);
		else
			uid = msg_uid[i+start];

		if (statusonly)
			continue;

		// Check if mail is already known
		if (new_mail (uid))
			continue;

		// TOP
		command_top (mail, start + i);

		// Parse mail
		parse (mail, uid);
	}
}

/**
 * Opening the socket for the connection to the server. If authentication is
 * set to autodetection before this is done. Login to the server is handled
 * by the methods Pop3::connect() or Apop::connect().
 *
 * @exception pop_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Pop::connect (void) throw (pop_err)
{
	// Autodetection of authentication
	if (authentication() == AUTH_AUTODETECT) {
		guint prt = port();
		if (!use_other_port())
			prt = 995;
		if (!socket_->open (address(), prt, AUTH_SSL)) {
			if (!use_other_port())
				prt = 110;
			if (!socket_->open (address(), prt, AUTH_USER_PASS))
				throw pop_socket_err();
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
		throw pop_socket_err();
}

/**
 * Sending the POP3 command "QUIT" to the server. If this succeeds the
 * connection to the POP3 server is closed.
 *
 * @exception pop_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Pop::command_quit (void) throw (pop_err)
{
	std::string line;

	// Sending the command
	sendline ("QUIT");
	readline (line, true, true, false);
	// Closing the socket
	socket_->close();
}

/**
 * Sending the POP3 command "STAT" to the server to get the total number of
 * messages.
 *
 * @return             Total number of messages
 * @exception pop_command_err
 *                     This exception is thrown if there is an error in the
 *                     server's response.
 * @exception pop_socket_err
 *                     This exception is thrown if a network error occurs.
 */
guint 
Pop::command_stat (void) throw (pop_err)
{
	std::string line;

	// Get total number of messages into total
	sendline ("STAT");
	readline (line);
	// line is "+OK total total_size" (see RFC 1939 5.)
	std::stringstream ss(line.substr(4));
	if (!g_ascii_isdigit(line[4])) throw pop_command_err();
	guint total;
	ss >> total;
	return total;
}

/**
 * Sending the POP3 command "TOP" to get the header and the first lines of a
 * mail.
 *
 * @param  mail        Lines of the obtained mail in a vector
 * @param  msg         Message number of the mail in question
 * @exception pop_command_err
 *                     This exception is thrown if there is an error in the
 *                     server's response.
 * @exception pop_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Pop::command_top (std::vector<std::string> &mail, guint msg) throw (pop_err)
{
	std::string line;

	// Clear old mail
	mail.clear ();

	std::stringstream ss;
	ss << "TOP " << msg << " " << biff_->value_uint ("min_body_lines");
	// Get header and first lines of mail
	sendline (ss.str ());
	readline (line, false); // +OK response to TOP
#ifdef DEBUG
	g_print ("** Message: [%d] RECV(%s:%d): (message) ", uin(),
			 address().c_str(), port());
#endif
	gint cnt = biff_->value_uint ("prevdos_header_lines");
	cnt += biff_->value_uint ("min_body_lines") + 1;
	do {
		readline (line, false, true, false);
		// Remove trailing '\n'
		if (line.size() > 0) {
			if (line[0]!='.')
				mail.push_back (line.substr(0, line.size()-1));
			else // Note: We know line.size()>1 in this case
				mail.push_back (line.substr(0, line.size()-2));
#ifdef DEBUG
			g_print ("+");
#endif
		}
		else throw pop_command_err ();
	} while ((line != ".\r") && (cnt--));
	if (cnt < 0) throw pop_dos_err();
#ifdef DEBUG
	g_print("\n");
#endif
	// Remove ".\r" line
	mail.pop_back();
}

/**
 * Sending the POP3 command "UIDL" to get the unique id for all mails.
 *
 * @param  total       Total number of messages
 * @param  msg_uid     Reference to a map of pairs (message number, unique id)
 *                     that is used to return the obtained values.
 * @exception pop_command_err
 *                     This exception is thrown if there is an error in the
 *                     server's response.
 * @exception pop_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Pop::command_uidl (guint total, std::map<guint,std::string> &msg_uid)
				   throw (pop_err)
{
	std::string line, uid;
	guint msg_int;
	msg_uid.clear ();

	// Send command
	sendline ("UIDL");
	readline (line); // line is "+OK" (see RFC 1939 7.)

	for (guint msg=1; msg <= total; msg++) {
		readline (line, true, true, false);
		std::stringstream ss(line);
		ss >> msg_int >> uid;
		if (msg_int != msg) throw pop_command_err ();
		if ((uid.size() > 70) || (uid.size() == 0)) throw pop_command_err ();
		msg_uid[msg] = uid;
	}
	readline (line, true, true, false); // line is ".\r"
	if (line != ".\r") throw pop_command_err ();
}

/**
 * Sending the POP3 command "UIDL" to get the unique id for a mail.
 *
 * @param  msg         Message number of the mail in question
 * @return             unique id as a C++ String
 * @exception pop_command_err
 *                     This exception is thrown if there is an error in the
 *                     server's response.
 * @exception pop_socket_err
 *                     This exception is thrown if a network error occurs.
 */
std::string 
Pop::command_uidl (guint msg) throw (pop_err)
{
	std::string line, uid;
	guint msg_int;

	std::stringstream ss_msg;
	ss_msg << msg;

	// Send command
	sendline ("UIDL " + ss_msg.str ());
	readline (line); // line is "+OK msg uidl" (see RFC 1939 7.)

	std::stringstream ss(line.substr(4));
	ss >> msg_int >> uid;
	if (msg_int != msg) throw pop_command_err ();
	if ((uid.size() > 70) || (uid.size() == 0)) throw pop_command_err ();

	return uid;
}

/**
 * Send a line to the POP server.
 * The given {\em line} is postfixed with "\r\n" and then written to
 * the socket of the mailbox.
 *
 * If {\em check} is true the return value of the call to Socket::write() is
 * checked and an pop_socket_err exception is thrown if it was not successful.
 * So this function always returns SOCKET_STATUS_OK if {\em check} is true,
 * otherwise (if {\em check} is false) error handling is left to the caller of
 * this function.
 *
 * @param line     line to be sent
 * @param print    Shall the sent command be printed in debug mode?
 *                 The default is true.
 * @param check    Shall the return value of the Socket::write() command be
 *                 checked? The default is true.
 * @return         Return value of the Socket::write() command, this is always
 *                 SOCKET_STATUS_OK if {\em check} is true.
 * @exception pop_socket_err
 *                 This exception is thrown if a network error occurs.
 */
gint 
Pop::sendline (const std::string line, gboolean print, gboolean check)
			   throw (pop_err)
{
	gint status=socket_->write (line + "\r\n", print);
	if ((status!=SOCKET_STATUS_OK) && check) throw pop_socket_err();
	return status;
}

/**
 * Read one line from the server. If {\em check} is true the return value of
 * the call to Socket::read() is checked and a pop_socket_err exception is
 * thrown if it was not successful. So this function always returns
 * SOCKET_STATUS_OK if {\em check} is true, otherwise (if {\em check} is
 * false) error handling is left to the caller of this function.
 *
 * If {\em checkline} is true then the read line is checked for an negative
 * response ("-ERR"). If such a response is found an error message is printed,
 * the "QUIT" command is sent (see remark below) and a pop_command_err
 * exception is thrown.
 *
 * Remark: The parameter {\em checkline} must be false if reading the response
 * to the "QUIT" command.
 *
 * @param line      String that contains the read line if the call was
 *                  successful (i.e. the return value is SOCKET_STATUS_OK),
 *                  the value is undetermined otherwise
 * @param print     Shall the read line be printed in debug mode?
 *                  The default is true.
 * @param check     Shall the return value of the Socket::read() command be
 *                  checked? The default is true.
 * @param checkline Shall {\em line} be checked for an error response?
 *                  The default is true.
 * @return          Return value of the Socket::read() command, this is always
 *                  SOCKET_STATUS_OK if {\em check} is true.
 * @exception pop_command_err
 *                  This exception is thrown if {\em line} contains a negative
 *                  response and {\em checkline} is true.
 * @exception pop_socket_err
 *                  This exception is thrown if a network error occurs.
 */
gint 
Pop::readline (std::string &line, gboolean print, gboolean check,
			   gboolean checkline) throw (pop_err)
{
	// Read line
	gint status=socket_->read(line, print, check);
	if (check && (status!=SOCKET_STATUS_OK)) throw pop_socket_err();

	// Only "+OK" and "-ERR" are valid responses (see RFC 1939 3.)
	if (!checkline)
		return status;
	if (line.find ("-ERR") == 0) {
		g_warning (_("[%d] Error message from POP3 server:%s"), uin(),
					 line.substr(4,line.size()-4).c_str());
		// We are still able to logout
		command_quit ();
		throw pop_command_err();
	}
	if (line.find ("+OK") != 0) {
		g_warning (_("[%d] Did not get a positive response from POP3 server"),
				   uin());
		throw pop_command_err();
	}
	return status;
}
