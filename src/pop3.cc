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

#include <string>
#include <sstream>
#include <glib.h>
#include <sys/stat.h>
#include <utime.h>

#include "ui-authentication.h"
#include "pop3.h"
#include "socket.h"
#include "nls.h"


Pop3::Pop3 (Biff *biff) : Pop (biff)
{
	protocol_ = PROTOCOL_POP3;
}

Pop3::Pop3 (const Mailbox &other) : Pop (other)
{
	protocol_ = PROTOCOL_POP3;
}

Pop3::~Pop3 (void)
{
	delete socket_;
}


int
Pop3::connect (void)
{
	if (biff_->no_clear_password_ && !use_ssl_) {
		status_  = MAILBOX_BLOCKED;
		return 0;
	}

	// show authentication if password is empty
	if (password_.empty())
		ui_authentication_->select (this);

	// if it is still empty after authentication, just return
	if (password_.empty()) {
		socket_->status (SOCKET_STATUS_ERROR);
		status_  = MAILBOX_ERROR;
		return 0;
	}

	// connection
	if (!socket_->open (hostname_, port_, use_ssl_, certificate_)) {
		socket_->status (SOCKET_STATUS_ERROR);
		status_ = MAILBOX_ERROR;
		return 0;
	}

	std::string line;
	if (!socket_->read (line)) return 0;

	// LOGIN : username
	line = "USER " + username_ + std::string ("\r\n");
	if (!socket_->write (line)) return 0;
	if (!socket_->read (line)) return 0;

	// LOGIN : password
	line = "PASS " + password_ + std::string ("\r\n");

	// Just in case send someone me the output: password won't be displayed
	std::string line_no_password = "PASS (hidden)\r\n";
#ifdef DEBUG
	g_print ("** Message: [%d] SEND(%s:%d): %s", uin_, hostname_.c_str(), port_, line_no_password.c_str());
#endif

	if (!socket_->write (line,false)) return 0;
	if (!socket_->read (line)) return 0;

	return 1;
}
