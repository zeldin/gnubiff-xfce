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

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#ifdef HAVE_CRYPTO
#  include <openssl/md5.h>
#endif

#include <string>
#include <sys/stat.h>
#include <utime.h>

#include "ui-authentication.h"
#include "socket.h"
#include "apop.h"
#include "nls.h"


Apop::Apop (Biff *biff) : Pop (biff)
{
	protocol_ = PROTOCOL_APOP;
}

Apop::Apop (const Mailbox &other) : Pop (other)
{
	protocol_ = PROTOCOL_APOP;
}

Apop::~Apop (void)
{
	delete socket_;
}

int
Apop::connect (void)
{
	std::string line;

	// Check password is not empty
	if (password_.empty())
		ui_authentication_->select (this);

	if (password_.empty()) {
		socket_->status(SOCKET_STATUS_ERROR);
		status_ = MAILBOX_ERROR;
		g_warning (_("[%d] Empty password"), uin_);
		return 0;
	}

	// Connection
	if (!socket_->open (hostname_, port_, use_ssl_, certificate_)) {
		socket_->status (SOCKET_STATUS_ERROR);
		status_ = MAILBOX_ERROR;
		return 0;
	}

	// Does server supports apop protocol ?
	//  if so, answer should be something like:
	//  +OK POP3 server ready <1896.697170952@dbc.mtview.ca.us>
	if (!socket_->read (line)) return 0;
	if (line.find ("<") == std::string::npos) {
		g_warning (_("[%d] Your pop server does not seem to accept apop protocol (no timestamp provided)"), uin_);
		socket_->status (SOCKET_STATUS_ERROR);
		status_ = MAILBOX_ERROR;
		return 0;
	}

	

	// Get time stamp from server
	std::string timestamp = line.substr (line.find ("<"));
	timestamp = timestamp.substr (0, timestamp.find (">")+1);

#ifdef DEBUG
	g_message ("[%d] timestamp is %s", uin_, timestamp.c_str());
#endif

	// Build message if MD5 library available
	char hex_response[33];
#ifdef HAVE_CRYPTO
	unsigned char response[16];
	MD5_CTX ctx;
	MD5_Init (&ctx);
	MD5_Update (&ctx, timestamp.c_str(), timestamp.size());
	MD5_Update (&ctx, password_.c_str(), password_.size());
	MD5_Final (response, &ctx);
	for (guint i = 0; i < 16; i++)
		sprintf (&hex_response[i*2], "%02x", response[i]);
	hex_response[32] = '\0';
#else
	g_message (_("[%d] Problem with crypto that should have been detected at configure time", uin_));
	return 0;
#endif

	// LOGIN
	line = "APOP " + username_ + std::string(" ") + std::string (hex_response) + std::string ("\r\n");
	if (!socket_->write (line)) return 0;
	if (!socket_->read (line)) return 0;

	return 1;
}
