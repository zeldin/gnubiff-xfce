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

// FIXME: I got these two defines from config.h.in but I found no
// smart way to have only one because I do not know how to do
// I would only need a have_libcrypto if we have libcrypto or libk5crypto
#ifdef HAVE_CRYPTO
#include <openssl/md5.h>
#endif


#include "Apop.h"


// ================================================================================
//  Constructors & Destructors
// --------------------------------------------------------------------------------
//  
// ================================================================================
Apop::Apop (Biff *owner) : Pop (owner)
{
	_protocol = PROTOCOL_APOP;
}

Apop::Apop (Mailbox *other) : Pop (other)
{
	_protocol = PROTOCOL_APOP;
}

Apop::~Apop (void)
{
}


// ================================================================================
//  Connection to an apop server
// --------------------------------------------------------------------------------
//  
// ================================================================================
void Apop::connect (void) {
	std::string line;

	// Check password is not empty
	if (_password.empty()) {
		_socket_status = SOCKET_STATUS_ERROR;
		_status = MAILBOX_ERROR;
		return;
	}

	// Connection
	socket_open (_address, _port);  CHECK_SOCKET;

	// Does server supports apop protocol ?
	//  if so, answer should be something like:
	//  +OK POP3 server ready <1896.697170952@dbc.mtview.ca.us>
	socket_read (line); CHECK_SERVER_RESPONSE;
	if (line.find ("<") == std::string::npos)
		return;

	// Get time stamp from server
	std::string timestamp = line.substr (line.find ("<")+1);
	timestamp = timestamp.substr(0, timestamp.size()-1);

	// Build message if MD5 library available
	char hex_response[33];
#ifdef HAVE_CRYPTO
	unsigned char response[16];
	MD5_CTX ctx;
	MD5_Init (&ctx);
	MD5_Update (&ctx, timestamp.c_str(), timestamp.size());
	MD5_Update (&ctx, _password.c_str(), _password.size());
	MD5_Final (response, &ctx);
	for (guint i = 0; i < 16; i++)
		sprintf (&hex_response[i*2], _("%02x"), response[i]);
	hex_response[32] = '\0';
#else
	return;
#endif

	// LOGIN
	line = "APOP " + _user + std::string (hex_response) + std::string ("\r\n");
	socket_write (line); CHECK_SOCKET;
	socket_read (line);  CHECK_SERVER_RESPONSE;
}
