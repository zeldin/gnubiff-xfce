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
#include "imap4.h"
#include "nls.h"
#include "support.h"


Imap4::Imap4 (Biff *biff) : Mailbox (biff)
{
	protocol_ = PROTOCOL_IMAP4;
	folder_   = "INBOX";
	socket_   = new Socket (this);
}

Imap4::Imap4 (const Mailbox &other) : Mailbox (other)
{
	protocol_ = PROTOCOL_IMAP4;
	socket_   = new Socket (this);
}

Imap4::~Imap4 (void)
{
	delete socket_;
}

gint Imap4::connect (void)
{
	if (biff_->no_clear_password_ && !use_ssl_) {
#ifdef DEBUG
		g_message ("[%d] Mailbox is blocked because it is unsecure", uin_);
#endif
		status_  = MAILBOX_BLOCKED;
		return 0;
	}

	std::string line;

#ifdef DEBUG
	g_message ("[%d] Trying to connect to %s on port %d", uin_, hostname_.c_str(), port_);
#endif

	// Connection
	if (!socket_->open (hostname_, port_, use_ssl_, certificate_)) {
		socket_->status (SOCKET_STATUS_ERROR);
		status_ = MAILBOX_ERROR;
		return 0;
	}

#ifdef DEBUG
	g_message ("[%d] Connected to %s on port %d", uin_, hostname_.c_str(), port_);
#endif
	if (!(socket_->read (line, true))) return 0;

	// LOGIN
	line = "A001 LOGIN \"" + username_ + std::string ("\" \"") + password_ + std::string ("\"\r\n");

	// Just in case send someone me the output: password won't be displayed
#ifdef DEBUG
	std::string line_no_password = "A001 LOGIN \"" + username_ + std::string ("\" \"")
		+ std::string("(hidden)") + std::string ("\"\r\n");
	g_message ("[%d] SEND(%s:%d): %s", uin_, hostname_.c_str(), port_, line_no_password.c_str());
#endif

	if (!socket_->write (line, false)) return 0;
	if (!(socket_->read (line))) return 0;
	if (line.find ("A001 OK") != 0) {
		socket_->status (SOCKET_STATUS_ERROR);
		status_ = MAILBOX_ERROR;
		g_warning (_("[%d] Unable to get acknowledgment from %s on port %d"), uin_, hostname_.c_str(), port_);
		return 0;
	}

	// SELECT
	gboolean check = false;
	gchar *folder_imaputf7=gb_utf8_to_imaputf7(folder_.c_str(),-1);
	if (folder_imaputf7)
	{
		std::string s = std::string("A002 SELECT \"") + folder_imaputf7
						+ std::string ("\"\r\n");
		g_free(folder_imaputf7);
		if (!socket_->write (s.c_str())) return 0;

		// We need to set a limit to lines read (DoS Attacks).
		// According to RFC 3501 6.3.1 there must be exactly seven lines
		// before the "A002 OK ..." line.
		gint cnt=8+preventDoS_additionalLines_;
		while ((socket_->read (line)) && (cnt--))
		{
			if (line.find ("A002 OK") == 0)
			{
				check = true;
				break;
			}
			else if (line.find ("A002") == 0)
			{
				socket_->write ("A003 LOGOUT\r\n");
				socket_->close ();
				break;
			}
		}
	}
	else
	{
		socket_->write ("A002 LOGOUT\r\n");
		socket_->close ();
	}

	if (!socket_->status()||!check||!folder_imaputf7)
	{
		socket_->status (SOCKET_STATUS_ERROR);
		status_ = MAILBOX_ERROR;
		g_warning (_("[%d] Unable to select folder %s on host %s"),
				   uin_, folder_.c_str(), hostname_.c_str());
		return 0;
	}

	socket_->status(SOCKET_STATUS_OK);

	return 1;
}


void
Imap4::get_status (void)
{
	std::string line;
	std::vector<int> buffer;
  
	// By default we consider to have an error status
	status_ = MAILBOX_CHECKING;

	// Check password is not empty
	if (password_.empty())
		ui_authentication_->select (this);

	if (password_.empty()) {
		status_ = MAILBOX_ERROR;
		return;
	}

	// Connection and authentification
	if (!connect ()) return;

	// SEARCH NOT SEEN
	if (!socket_->write ("A003 SEARCH NOT SEEN\r\n")) return;

	// We need to set a limit to lines read (DoS Attacks).
	// Expected response "* SEARCH ..." should be in the next line.
	gint cnt=1+preventDoS_additionalLines_;
	while (((socket_->read(line) > 0)) && (cnt--))
		if (line.find ("* SEARCH") == 0)
			break;
	if ((!socket_->status()) || (cnt<0)) return;


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
		status_ = MAILBOX_EMPTY;

	// Quick test (when there were really no change at all)
	else if (buffer == saved_)
		status_ = MAILBOX_OLD;

	// Quick test (if there are only more mail than previously)
	else if (buffer.size() > saved_.size())
		status_ = MAILBOX_NEW;

	// Slow test (same size because it may happen we read one
	// email from elsewhere but there is also a new one)
	else {
		status_ = MAILBOX_OLD;
		guint i, j;
		for (i=0; i<buffer.size(); i++) {
			for (j=0; j<saved_.size(); j++) {
				if (buffer[i] == saved_[j])
					break;
			}
			if (j == saved_.size()) {
				status_ = MAILBOX_NEW;
				break;
			}
		}
	}

	// We're done
	saved_ = buffer;
	cnt=1+preventDoS_additionalLines_;
	while ((socket_->read (line) > 0) && (cnt--))
		if (line.find ("A003") != std::string::npos)
			break;
	if ((!socket_->status()) || (cnt<0)) return;

	// Closing connection
	if (!socket_->write ("A004 LOGOUT\r\n")) return;
	socket_->close ();
}

void
Imap4::get_header (void)
{
	std::string line;
	std::vector<int> buffer;
	int saved_status = status_;
  
	// Status will be restored in the end if no problem occured
	status_ = MAILBOX_CHECKING;

	// Check password is not empty
	if (password_.empty())
		ui_authentication_->select (this);

	if (password_.empty()) {
		status_ = MAILBOX_ERROR;
		return;
	}

	// Connection and authentification
	if (!connect ()) {
		status_ = MAILBOX_ERROR;
		return;
	}

	// SEARCH NOT SEEN
	if (!socket_->write ("A003 SEARCH NOT SEEN\r\n")) return;

	// We need to set a limit to lines read (DoS Attacks).
	// Expected response "* SEARCH ..." should be in the next line.
	gint cnt=1+preventDoS_additionalLines_;
	while (((socket_->read(line) > 0)) && (cnt--))
		if (line.find ("* SEARCH") == 0)
			break;
	if ((!socket_->status()) || (cnt<0)) return;

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

	cnt=1+preventDoS_additionalLines_;
	while ((socket_->read (line) > 0) && (cnt--))
		if (line.find ("A003") != std::string::npos)
			break;
	if ((!socket_->status()) || (cnt<0)) return;


	// FETCH NOT SEEN
	new_unread_.clear();
	new_seen_.clear();
	std::vector<std::string> mail;
	for (guint i=0; (i<buffer.size()) && (unread_.size() < (unsigned int)(biff_->max_mail_)); i++) {
		std::stringstream s;
		s << buffer[i];
		mail.clear();

		std::string line = std::string ("A004 FETCH ") + s.str() + std::string (" (FLAGS BODY.PEEK[HEADER.FIELDS (DATE FROM SUBJECT)])\r\n");
		if (!socket_->write (line)) return;
		if (!socket_->read (line)) return;
		if (!socket_->read (line,false)) return;
#ifdef DEBUG
		g_print ("** Message: [%d] RECV(%s:%d): (message) ", uin_, hostname_.c_str(), port_);
#endif
		while (line.find ("A004 OK") == std::string::npos) {
			if (line.size() > 0) {
				mail.push_back (line.substr(0, line.size()-1));
#ifdef DEBUG
				g_print ("+");
#endif
			}
			if (!socket_->read (line, false)) return;
		}
#ifdef DEBUG
		g_print ("\n");
#endif

		// Remove two last lines which are an empty line and a line with a closing parenthesis
		mail.pop_back();
		mail.pop_back();

		line = std::string ("A005 FETCH ") + s.str() + std::string (" (FLAGS BODY.PEEK[TEXT])\r\n");

		if (!socket_->write (line)) return;
		if (!socket_->read (line)) return;
		if (!socket_->read (line,false)) return;
#ifdef DEBUG
		g_print ("** Message: [%d] RECV(%s:%d): (message) ", uin_, hostname_.c_str(), port_);
#endif
		guint j = 0;
		while (line.find ("A005 OK") == std::string::npos) {
			if (j < 13) {
				if (line.size() > 0) {
					mail.push_back (line.substr(0, line.size()-1));
#ifdef DEBUG
					g_print ("+");
#endif
				}
				j++;
			}
			if (!socket_->read (line, false)) return;
		}
#ifdef DEBUG
		g_print("\n");
#endif

		// Remove two last lines which are an empty line and a line with a closing parenthesis
		mail.pop_back();
		mail.pop_back();
		parse (mail, MAIL_UNREAD);
	}

	// Closing connection
	if (!socket_->write ("A006 LOGOUT\r\n")) return;
	socket_->close ();

	// We restore status
	status_ = saved_status;

	if ((unread_ != new_unread_) && (new_unread_.size() > 0))
		status_ = MAILBOX_NEW;
	else
		status_ = MAILBOX_OLD;

	unread_ = new_unread_;
	seen_ = new_seen_;
}
