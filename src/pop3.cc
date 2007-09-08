// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2007 Nicolas Rougier, 2004-2007 Robert Sowada
//
// This program is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ========================================================================
//
// File          : $RCSfile$
// Revision      : $Revision$
// Revision date : $Date$
// Author(s)     : Nicolas Rougier, Robert Sowada
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
#include "pop3.h"
#include "socket.h"

// ========================================================================
//  base
// ========================================================================	
/**
 * Constructor. The mailbox for the POP3 protocol is created from scratch.
 *
 * @param biff Pointer to the instance of Gnubiff.
 */
Pop3::Pop3 (Biff *biff) : Pop (biff)
{
	value ("protocol", PROTOCOL_POP3);
}

/**
 * Constructor. The mailbox for the POP3 protocol is created by taking the
 * attributes of the existing mailbox {\em other}.
 *
 * @param other Mailbox from which the attributes are taken.
 */
Pop3::Pop3 (const Mailbox &other) : Pop (other)
{
	value ("protocol", PROTOCOL_POP3);
}

/// Destructor
Pop3::~Pop3 (void)
{
}


// ========================================================================
//  main
// ========================================================================	
/**
 * A connection to the mailbox is established. If this can't be done then a
 * {\em pop_socket_err} is thrown. Otherwise gnubiff logins.
 *
 * @exception pop_command_err
 *                     This exception is thrown when we get an unexpected
 *                     response.
 * @exception imap_socket_err
 *                     This exception is thrown if a network error occurs.
 */
void 
Pop3::connect (void) throw (pop_err)
{
	std::string line;

	// Open the socket
	Pop::connect ();

	readline (line); // +OK response

	// LOGIN: username
	sendline ("USER " + username());
	readline (line); // +OK response

	// LOGIN: password
	sendline ("PASS " + password(), false);
	readline (line); // +OK response
#ifdef DEBUG
	// Just in case someone sends me the output: password won't be displayed
	std::string line_no_password = "PASS (hidden)\r\n";
	g_message ("[%d] SEND(%s:%d): %s", uin(), address().c_str(), port(),
			   line_no_password.c_str());
#endif
}
