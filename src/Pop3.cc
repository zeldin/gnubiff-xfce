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
#include "Pop3.h"


// ================================================================================
//  Constructors & Destructors
// --------------------------------------------------------------------------------
//  
// ================================================================================
Pop3::Pop3 (Biff *owner) : Pop (owner)
{
	_protocol = PROTOCOL_POP3;
}

Pop3::Pop3 (Mailbox *other) : Pop (other)
{
	_protocol = PROTOCOL_POP3;
}

Pop3::~Pop3 (void)
{
}


// ================================================================================
//  Connection to a pop3 server
// --------------------------------------------------------------------------------
//  
// ================================================================================
void Pop3::connect (void) {
	std::string line;

	// Check password is not empty
	if (_password.empty()) {
		_socket_status = SOCKET_STATUS_ERROR;
		_status  = MAILBOX_ERROR;
		return;
	}

	// Connection
	socket_open (_address, _port);	CHECK_SOCKET;
	socket_read (line);				CHECK_SERVER_RESPONSE;

	// LOGIN : username
	line = "USER " + _user + std::string ("\r\n");
	socket_write (line);			CHECK_SOCKET;
	socket_read (line);				CHECK_SERVER_RESPONSE;

	// LOGIN : password
	line = "PASS " + _password + std::string ("\r\n");
	// Just in case send someone me the output: password won't be displayed
	std::string line_no_password = "PASS (hidden)\r\n";
#ifdef DEBUG
	g_message ("SEND(%s:%d): %s", _address.c_str(), _port, line_no_password.c_str());
#endif



	socket_write (line,true);			CHECK_SOCKET;
	socket_read (line);				CHECK_SERVER_RESPONSE;
}
