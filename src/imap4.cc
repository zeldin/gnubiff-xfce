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
	// Is there already a timeout ?
	if (timetag_)
		return;

	// Do we want to start using given delay ?
	if (delay)
		timetag_ = g_timeout_add (delay*1000, start_delayed_entry_point, this);
	//  or internal delay ?
	else
		timetag_ = g_timeout_add (delay_*1000, start_delayed_entry_point, this);
}

void
Imap4::start (void)
{
	if (!g_mutex_trylock (monitor_mutex_))
		return;
	fetch ();
	g_mutex_unlock (monitor_mutex_);

	threaded_start (delay_);
}

void
Imap4::fetch (void)
{
	fetch_status();
	if ((status_ == MAILBOX_NEW) || (status_ == MAILBOX_EMPTY))
		fetch_header();

	if (!GTK_WIDGET_VISIBLE (biff_->popup()->get())) {
		gdk_threads_enter();
		biff_->applet()->update();
		gdk_threads_leave();
	}
}

gint Imap4::connect (void)
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


#ifdef DEBUG
	g_message ("[%d] Connected to %s on port %d", uin_, address_.c_str(), port_);
#endif
	if (!(socket_->read (line, true))) return 0;

	// Get the IDLE capacity
	line = "A001 CAPABILITY" + std::string ("\r\n");
	if (!socket_->write (line, false)) return 0;
	if (!(socket_->read (line))) return 0;
	if (line.find ("IDLE") != std::string::npos)
		idleable_ = true;

	if (!(socket_->read (line))) return 0;
	if (line.find ("A001 OK") != 0) {
		socket_->status (SOCKET_STATUS_ERROR);
		status_ = MAILBOX_ERROR;
		g_warning (_("[%d] Unable to get acknowledgment from %s on port %d"), uin_, address_.c_str(), port_);
		return 0;
	}


	// LOGIN
	line = "A002 LOGIN \"" + username_ + std::string ("\" \"") + password_ + std::string ("\"\r\n");

	// Just in case send someone me the output: password won't be displayed
#ifdef DEBUG
	std::string line_no_password = "A002 LOGIN \"" + username_ + std::string ("\" \"")
		+ std::string("(hidden)") + std::string ("\"\r\n");
	g_message ("[%d] SEND(%s:%d): %s", uin_, address_.c_str(), port_, line_no_password.c_str());
#endif

	if (!socket_->write (line, false)) return 0;
	if (!(socket_->read (line))) return 0;
	if (line.find ("A002 OK") != 0) {
		socket_->status (SOCKET_STATUS_ERROR);
		status_ = MAILBOX_ERROR;
		g_warning (_("[%d] Unable to get acknowledgment from %s on port %d"), uin_, address_.c_str(), port_);
		return 0;
	}

	// SELECT
	std::string s = std::string("A003 SELECT \"")+ folder_ + std::string ("\"\r\n");
	if (!socket_->write (s.c_str())) return 0;

	gboolean check = FALSE;
	while (socket_->read (line)) {
		if (line.find ("A003 OK") != std::string::npos) {
			check = true;
			break;
		}
		else if (line.find ("A003") != std::string::npos)
			break;
	}

	if (!socket_->status()) return 0;

	if (check)
		socket_->status(SOCKET_STATUS_OK);

	return 1;
}


void
Imap4::fetch_status (void)
{
	std::string line;
	std::vector<int> buffer;
  
	// By default we consider to have an error status
	status_ = MAILBOX_CHECK;

	// Check password is not empty
	if (password_.empty()) {
		gdk_threads_enter ();
		ui_auth_->select (this);
		gdk_threads_leave ();
	}

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

	while ((socket_->read(line) > 0))
		if (line.find ("* SEARCH") == 0)
			break;
	if (!socket_->status()) return;


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
	while ((socket_->read (line) > 0))
		if (line.find ("A003") != std::string::npos)
			break;
	if (!socket_->status()) return;

	// Closing connection
	if (!socket_->write ("A004 LOGOUT\r\n")) return;
	socket_->close ();
}

void
Imap4::fetch_header (void)
{
	std::string line;
	std::vector<int> buffer;
	int saved_status = status_;
	gboolean idling = true;
  
	// Status will be restored in the end if no problem occured
	status_ = MAILBOX_CHECK;

	// Check password is not empty
	if (password_.empty())
		ui_auth_->select (this);

	if (password_.empty()) {
		status_ = MAILBOX_ERROR;
		return;
	}

	// Connection and authentification
	if (!connect ()) {
		status_ = MAILBOX_ERROR;
		return;
	}

	// This loop is for the idle state when we wait for new message
	do {
		// SEARCH NOT SEEN
		if (!socket_->write ("A003 SEARCH NOT SEEN\r\n")) break;

		while (socket_->read(line))
			if (line.find ("* SEARCH") == 0)
				break;
		if (!socket_->status()) break;

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

		while ((socket_->read(line) > 0))
			if (line.find ("A003") != std::string::npos)
				break;
		if (!socket_->status()) break;


		// FETCH NOT SEEN
		new_unread_.clear();
		new_seen_.clear();
		std::vector<std::string> mail;
		for (guint i=0; (i<buffer.size()) && (new_unread_.size() < (unsigned int)(biff_->max_mail_)); i++) {
			std::stringstream s;
			s << buffer[i];
			mail.clear();

			std::string line = std::string ("A004 FETCH ") + s.str() + std::string (" (FLAGS BODY.PEEK[HEADER.FIELDS (DATE FROM SUBJECT)])\r\n");
			if (!socket_->write (line)) break;
			if (!socket_->read (line)) break;
			if (!socket_->read (line,false)) break;
#ifdef DEBUG
			g_print ("** Message: [%d] RECV(%s:%d): (message) ", uin_, address_.c_str(), port_);
#endif
			while (line.find ("A004 OK") == std::string::npos) {
				if (line.size() > 0) {
					mail.push_back (line.substr(0, line.size()-1));
#ifdef DEBUG
					g_print ("+");
#endif
				}
				if (!socket_->read (line, false)) break;
			}
#ifdef DEBUG
			g_print ("\n");
#endif

			if (!socket_->status()) break;

			// Remove two last lines which are an empty line and a line with a closing parenthesis
			mail.pop_back();
			mail.pop_back();

			line = std::string ("A005 FETCH ") + s.str() + std::string (" (FLAGS BODY.PEEK[TEXT])\r\n");

			if (!socket_->write (line)) break;
			if (!socket_->read (line)) break;
			if (!socket_->read (line,false)) break;
#ifdef DEBUG
			g_print ("** Message: [%d] RECV(%s:%d): (message) ", uin_, address_.c_str(), port_);
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
				if (!socket_->read (line, false)) break;
			}
#ifdef DEBUG
			g_print("\n");
#endif
			if (!socket_->status()) break;

			// Remove two last lines which are an empty line and a line with a closing parenthesis
			if (j > 3) {
				mail.pop_back();
				mail.pop_back();
			}
			else {
				mail.push_back (_("<EMPTY>"));
			}
			parse (mail, MAIL_UNREAD);
		}
		if (!socket_->status()) break;


		// We restore status
		status_ = saved_status;

		if ((unread_ != new_unread_) && (new_unread_.size() > 0))
			status_ = MAILBOX_NEW;
		else if (new_unread_.size() == 0)
			status_ = MAILBOX_EMPTY;
		else
			status_ = MAILBOX_OLD;

		unread_ = new_unread_;
		seen_ = new_seen_;

		// Is server idleable ?
		if (idleable_) {

			// When in idle state, we won't exit this thread function
			// so we have to, update applet in the meantime
			if (!GTK_WIDGET_VISIBLE (biff_->popup()->get())) {
				gdk_threads_enter();
				biff_->applet()->update();
				gdk_threads_leave();
			}

			if (timetag_)
				g_source_remove (timetag_);
			timetag_ = 0;
			// Entering idle state
			line = std::string ("A006 IDLE") +std::string ("\r\n");
			if (!socket_->write (line)) break;
		
			// Read acknowledgement
			if (!socket_->read (line)) break;

			// Wait for new mail
			if (!socket_->read (line)) break;

			line = std::string ("DONE") +std::string ("\r\n");
			if (!socket_->write (line)) idling = false;
		
			do {
				if (!socket_->read (line)) break;
			} while (line.find ("A006 OK") != 0);

		
		}
		else {
			// Closing connection
			if (!socket_->write ("A006 LOGOUT\r\n")) break;
			socket_->close ();
			idling = false;
		}
		if (!socket_->status()) break;
	} while (idling);

}
