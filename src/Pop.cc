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
#include <sys/stat.h>
#include <utime.h>
#include "Pop.h"


// ================================================================================
//  Some useful macros (as always)
// ================================================================================
#ifdef CHECK_SOCKET
#  undef CHECK_SOCKET
#endif
#define CHECK_SOCKET		if (_socket_status == SOCKET_STATUS_ERROR) {socket_close(); _status = MAILBOX_ERROR; return;}


// ================================================================================
//  Constructors & Destructors
// --------------------------------------------------------------------------------
//  
// ================================================================================
Pop::Pop (Biff *owner) : Mailbox (owner)
{
}

Pop::Pop (Mailbox *other) : Mailbox (other)
{
}

Pop::~Pop (void)
{
}


// ================================================================================
//  Try to quickly get mailbox status
// --------------------------------------------------------------------------------
//
// ================================================================================
void Pop::get_status (void)
{
	std::string line;
	std::vector<std::string> buffer;
  
	// By default we consider to be in an error status
	_status = MAILBOX_CHECKING;

	// Connection and authentification
	connect(); CHECK_SOCKET;

	// Get UIDL list
	line = "UIDL\r\n";
	socket_write (line); CHECK_SOCKET;
	socket_read (line);  CHECK_SERVER_RESPONSE;

	buffer.clear();
	do {
		int i;
		char uidl[71];
		socket_read (line); CHECK_SOCKET;
		if (line[0] != '.') {
			sscanf (line.c_str(), "%d %s\n", &i, (char *) &uidl);
			buffer.push_back(uidl);
		}
	} while (line[0] != '.');


	// Find mailbox status by comparing saved uidl list with the new one
	if (buffer.empty())
		_status = MAILBOX_EMPTY;
	else if (buffer == _saved)
		_status = MAILBOX_OLD;
	else {
		_status = MAILBOX_OLD;
		guint i, j;
		for (i=0; i<buffer.size(); i++) {
			for (j=0; j<_saved.size(); j++) {
				if (buffer[i] == _saved[j])
					break;
			}
			if (j == _saved.size()) {
				_status = MAILBOX_NEW;
				break;
			}
		}
	}
	_saved = buffer;


	// LOGOUT
	line = "QUIT\r\n";
	socket_write (line);
	if (_socket_status == SOCKET_STATUS_ERROR) {
		socket_close();
		_status = MAILBOX_ERROR;
		return;
	}
	socket_read (line);
	if (_socket_status == SOCKET_STATUS_ERROR) {
		socket_close();
		_status = MAILBOX_ERROR;
		return;
	}
	if (line.find ("-ERR") == 0) {
		_socket_status = SOCKET_STATUS_ERROR;
		_status = MAILBOX_ERROR;
		return;
	}

	socket_close();
}


// ================================================================================
//  Get headers
// --------------------------------------------------------------------------------
//  
// ================================================================================
void Pop::get_header (void)
{
	std::string line;
	static std::vector<std::string> buffer;
	int saved_status = _status;
  
	// Status will be restored in the end if no problem occured
	_status = MAILBOX_CHECKING;

	// Connection and authentification
	connect(); CHECK_SOCKET;

	// STAT
	line = "STAT\r\n";
	socket_write (line); CHECK_SOCKET;
	socket_read (line);  CHECK_SOCKET;
	guint n;
	sscanf (line.c_str(),  "+OK %d", &n);

	// Fetch mails
	std::vector<header> old_unread = _unread;
	_unread.clear();
	_seen.clear();
	std::vector<std::string> mail;
	for (guint i=0; (i<n) && (_unread.size() < (unsigned int)(_owner->_max_collected_mail)); i++) {
		std::stringstream s;
		s << (i+1);

		// Get header and first 12 lines of mail
		line = "TOP " + s.str() + std::string (" 12\r\n");
		socket_write (line); CHECK_SOCKET;
		socket_read (line);  CHECK_SERVER_RESPONSE;
		do {
			socket_read (line); CHECK_SOCKET;
			mail.push_back (line.substr(0, line.size()-1));
		} while (line[0] != '.');
		mail.pop_back();
		parse (mail, MAIL_UNREAD);
		mail.clear();
	}


	// LOGOUT
	line = "QUIT\r\n";
	socket_write (line);
	CHECK_SOCKET;
	socket_close();

	// Restore status
	_status = saved_status;

	// Last check for mailbox status
	if ((_unread == old_unread) && (_unread.size() > 0))
		_status = MAILBOX_OLD;
}
