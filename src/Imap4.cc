/* gnubiff -- a mail notification program
 * Copyright (c) 2000-2004 Nicolas Rougier
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * This file is part of gnubiff.
 */
#include <string>
#include <sstream>
#include <glib.h>
#include <sys/stat.h>
#include <utime.h>
#include "Imap4.h"


// ================================================================================
//  Constructors & Destructors
// --------------------------------------------------------------------------------
//  
// ================================================================================
Imap4::Imap4 (Biff *owner) : Mailbox (owner)
{
	_protocol = PROTOCOL_IMAP4;
	_folder = "INBOX";
}

Imap4::Imap4 (Mailbox *other) : Mailbox (other)
{
	_protocol = PROTOCOL_IMAP4;
	_folder = "INBOX";
}

Imap4::~Imap4 (void)
{
}


// ================================================================================
//  Connection to an imap4 server
// --------------------------------------------------------------------------------
//  
// ================================================================================
gint Imap4::connect (void)
{
	std::string line;

#ifdef DEBUG
	g_message ("Trying to connect to %s on port %d", _address.c_str(), _port);
#endif

	// Connection
	if (!socket_open (_address, _port)) {
		socket_close();
		_status = MAILBOX_ERROR;
		return 0;
	}

#ifdef DEBUG
	g_message ("Connected to %s on port %d", _address.c_str(), _port);
#endif
	if (socket_read (line) == -1) {
		socket_close();
		_status = MAILBOX_ERROR;
		return 0;
	}

	// LOGIN
	line = "A001 LOGIN \"" + _user + std::string ("\" \"") + _password + std::string ("\"\r\n");

	// Just in case send someone me the output: password won't be displayed
#ifdef DEBUG
	std::string line_no_password = "A001 LOGIN \"" + _user + std::string ("\" \"")
		+ std::string("(hidden)") + std::string ("\"\r\n");
	g_message ("SEND(%s:%d): %s", _address.c_str(), _port, line_no_password.c_str());
#endif

	if (!socket_write (line, true)) {
		socket_close();
		_status = MAILBOX_ERROR;
		return 0;
	}
	if (socket_read (line) == -1) {
		socket_close();
		_status = MAILBOX_ERROR;
		return 0;
	}

	if (line.find ("A001 OK") != 0) {
		_socket_status = SOCKET_STATUS_ERROR;
		_status = MAILBOX_ERROR;
		g_warning (_("Unable to get acknowledgment from %s on port %d"), _address.c_str(), _port);
		return 0;
	}

	// SELECT
	std::string s = std::string("A002 SELECT \"")+ _folder + std::string ("\"\r\n");
	if (!socket_write (s.c_str())) {
		socket_close();
		_status = MAILBOX_ERROR;
		return 0;
	}

	gboolean check = FALSE;
	while ((socket_read (line) > 0) && (_socket_status != SOCKET_STATUS_ERROR))
		if (line.find ("A002 OK") != std::string::npos) {
			check = true;
			break;
		}
		else if (line.find ("A002") != std::string::npos) {
			break;
		}
	if (_socket_status == SOCKET_STATUS_ERROR) {
		socket_close();
		_status = MAILBOX_ERROR;
		return 0;
	}

	if (check)
		_socket_status = SOCKET_STATUS_OK;

	return 1;
}


// ================================================================================
//  Try to quickly get mailbox status
// --------------------------------------------------------------------------------
//
// ================================================================================
void Imap4::get_status (void)
{
	std::string line;
	std::vector<int> buffer;
  
	// By default we consider to have an error status
	_status = MAILBOX_CHECKING;

	// Check password is not empty
	if (_password.empty()) {
		_status = MAILBOX_ERROR;
		return;
	}

	// Connection and authentification
	if (!connect ()) {
		_status = MAILBOX_ERROR;
		return;
	}

	// SEARCH NOT SEEN
	if (!socket_write ("A003 SEARCH NOT SEEN\r\n")) {
		socket_close();
		_status = MAILBOX_ERROR;
		return;
	}
	while ((socket_read(line) > 0) && (_socket_status != SOCKET_STATUS_ERROR))
		if (line.find ("* SEARCH") == 0)
			break;
	if (_socket_status == SOCKET_STATUS_ERROR) {
		socket_close();
		_status = MAILBOX_ERROR;
		return;
	}


	// Parse server answer
	// Should be something like
	// "* SEARCH 1 2 3 4" or "* SEARCH"
	// (9 is size of "* SEARCH ")
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
	if (buffer.empty())
		_status = MAILBOX_EMPTY;
	else if (buffer == _saved)
		_status = MAILBOX_OLD;
	else
		_status = MAILBOX_NEW;

	// We're done
	_saved = buffer;
	while ((socket_read (line) > 0) && (_socket_status == SOCKET_STATUS_ERROR))
		if (line.find ("A003") != std::string::npos)
			break;
	if (_socket_status == SOCKET_STATUS_ERROR) {
		socket_close();
		_status = MAILBOX_ERROR;
		return;
	}

	// Closing connection
	socket_write ("A004 LOGOUT\r\n");
	if (_socket_status == SOCKET_STATUS_ERROR) {
		socket_close();
		_status = MAILBOX_ERROR;
		return;
	}
	socket_close ();
}


// ================================================================================
//  Get headers
// --------------------------------------------------------------------------------
//  
// ================================================================================
void Imap4::get_header (void) {
	std::string line;
	std::vector<int> buffer;
	int saved_status = _status;
  
	// Status will be restored in the end if no problem occured
	_status = MAILBOX_CHECKING;

	// Check password is not empty
	if (_password.empty()) {
		_status = MAILBOX_ERROR;
		return;
	}

	// Connection and authentification
	if (!connect ()) {
		_status = MAILBOX_ERROR;
		return;
	}

	// SEARCH NOT SEEN
	if (!socket_write ("A003 SEARCH NOT SEEN\r\n")) {
		socket_close();
		_status = MAILBOX_ERROR;
		return;
	}
	while ((socket_read(line) > 0) && (_socket_status != SOCKET_STATUS_ERROR))
		if (line.find ("* SEARCH") == 0)
			break;
	if (_socket_status == SOCKET_STATUS_ERROR) {
		socket_close();
		_status = MAILBOX_ERROR;
		return;
	}

	// Parse server answer
	// Should be something like
	// "* SEARCH 1 2 3 4" or "* SEARCH"
	// (9 is size of "* SEARCH ")
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

	// CHECK_SERVER_TAG("A003");
	while ((socket_read(line) > 0) && (_socket_status != SOCKET_STATUS_ERROR))
		if (line.find ("A003") != std::string::npos)
			break;
	if (_socket_status == SOCKET_STATUS_ERROR) {
		socket_close();
		_status = MAILBOX_ERROR;
		return;
	}


	// FETCH NOT SEEN
	std::vector<header> old_unread = _unread;
	_unread.clear();
	_seen.clear();
	std::vector<std::string> mail;
	for (guint i=0; (i<buffer.size()) && (_unread.size() < (unsigned int)(_owner->_max_collected_mail)); i++) {
		std::stringstream s;
		s << buffer[i];
		mail.clear();

		std::string line = std::string ("A004 FETCH ") + s.str() + std::string (" (FLAGS BODY.PEEK[HEADER.FIELDS (DATE FROM SUBJECT)])\r\n");
		if (!socket_write (line)) {
			socket_close();
			_status = MAILBOX_ERROR;
			return;
		}
		if (socket_read (line) == -1) {
			socket_close();
			_status = MAILBOX_ERROR;
			return;
		}
		if (socket_read (line) == -1) {
			socket_close();
			_status = MAILBOX_ERROR;
			return;
		}
		while (line.find ("A004 OK") == std::string::npos) {
			mail.push_back (line.substr(0, line.size()-1));
			if (socket_read (line) == -1) {
				socket_close();
				_status = MAILBOX_ERROR;
				return;
			}

		}
		// Remove two last lines which are an empty line and a line with a closing parenthesis
		mail.pop_back();
		mail.pop_back();

		line = std::string ("A005 FETCH ") + s.str() + std::string (" (FLAGS BODY.PEEK[TEXT])\r\n");

		if (!socket_write (line)) {
			socket_close();
			_status = MAILBOX_ERROR;
			return;
		}
		if (socket_read (line) == -1) {
			socket_close();
			_status = MAILBOX_ERROR;
			return;
		}
		if (socket_read (line) == -1) {
			socket_close();
			_status = MAILBOX_ERROR;
			return;
		}

		guint j = 0;
		while (line.find ("A005 OK") == std::string::npos) {
			if (j < 13) {
				mail.push_back (line.substr(0, line.size()-1));
				j++;
			}
			if (socket_read (line) == -1) {
				socket_close();
				_status = MAILBOX_ERROR;
				return;
			}
		}
		// Remove two last lines which are an empty line and a line with a closing parenthesis
		mail.pop_back();
		mail.pop_back();
		parse (mail, MAIL_UNREAD);
	}

	// Closing connection
	if (!socket_write ("A006 LOGOUT\r\n")) {
		socket_close();
		_status = MAILBOX_ERROR;
		return;
	}
	if (socket_read (line) == -1) {
		socket_close();
		_status = MAILBOX_ERROR;
		return;
	}
	socket_close ();

	// We restore status
	_status = saved_status;

	if ((_unread != old_unread) && (_unread.size() > 0))
		_status = MAILBOX_NEW;
	else
		_status = MAILBOX_OLD;
}
