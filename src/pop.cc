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

#include <sstream>
#include <sys/stat.h>
#include <utime.h>

#include "ui-applet.h"
#include "ui-popup.h"
#include "pop.h"
#include "socket.h"


// ========================================================================
//  base
// ========================================================================	
Pop::Pop (Biff *biff) : Mailbox (biff)
{
	socket_ = new Socket (this);
}

Pop::Pop (const Mailbox &other) : Mailbox (other)
{
	socket_ = new Socket (this);
}

Pop::~Pop (void)
{
}

// ========================================================================
//  main
// ========================================================================	
void
Pop::threaded_start (guint delay)
{
	stopped_ = false;

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
Pop::start (void)
{
	if (!g_mutex_trylock (monitor_mutex_))
		return;
	fetch ();
	g_mutex_unlock (monitor_mutex_);

	threaded_start (delay_);
}

void
Pop::fetch (void)
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

void
Pop::fetch_status (void)
{
	std::string line;
	std::vector<std::string> buffer;
	status_ = MAILBOX_CHECK;

	// Connection and authentification
	if (!connect())	return;

	// Get total number of messages into total
	line = "STAT\r\n";
	if (!socket_->write (line))	return;
	if (!socket_->read (line)) return;

	guint total;
	sscanf (line.c_str()+4, "%ud\n", &total);

	// We want to retrieve a maximum of _max_collected_mail uidl
	// so we have to check  total number and find corresponding
	// starting index (start).
	guint n;
	guint start;
	if (total > biff_->max_mail_) {
		n = biff_->max_mail_;
		start = 1 + total -  biff_->max_mail_;
	}
	else {
		n = total;
		start = 1;
	}

	// Retrieve uidl one by one to avoid to get all of them
	buffer.clear();
	char uidl[71];
	guint dummy;
	for (guint i=0; i< n; i++) {
		std::stringstream s;
		s << (i+start);
		line = "UIDL " + s.str() + std::string("\r\n");
		if (!socket_->write (line)) return;
		if (!socket_->read (line, false)) return;
#ifdef DEBUG
		g_print ("** Message: [%d] RECV(%s:%d): %s\n", uin_, address_.c_str(), port_, line.c_str());
#endif
		sscanf (line.c_str()+4, "%ud %70s\n", &dummy, (char *) &uidl);
		buffer.push_back (uidl);
	}

	// Find mailbox status by comparing saved uidl list with the new one
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
	saved_ = buffer;

	// LOGOUT
	line = "QUIT\r\n";
	if (!socket_->write (line)) return;
	if (!socket_->read (line)) return;

	socket_->close();
}


void
Pop::fetch_header (void)
{
	std::string line;
	static std::vector<std::string> buffer;
	int saved_status = status_;
  
	// Status will be restored in the end if no problem occured
	status_ = MAILBOX_CHECK;

	// Connection and authentification
	if (!connect()) return;

	// STAT
	line = "STAT\r\n";
	if (!socket_->write (line)) return;
	if (!socket_->read (line)) return;
	guint total;
	sscanf (line.c_str()+4, "%ud", &total);

	// We want to retrieve a maximum of _max_collected_mail 
	// so we have to check total number and find corresponding
	// starting index (start).
	guint n;
	guint start;
	if (total > biff_->max_mail_) {
		n = biff_->max_mail_;
		start = 1 + total - biff_->max_mail_;
	}
	else {
		n = total;
		start = 1;
	}

	// Fetch mails
	new_unread_.clear();
	new_seen_.clear();
	std::vector<std::string> mail;
	for (guint i=0; i < n; i++) {
		std::stringstream s;
		s << (i+start);
		mail.clear();
		// Get header and first 12 lines of mail
		line = "TOP " + s.str() + std::string (" 12\r\n");
		if (!socket_->write (line)) return;
#ifdef DEBUG
		g_print ("** Message: [%d] RECV(%s:%d): (message) ", uin_, address_.c_str(), port_);
#endif
		if (!socket_->read (line, false)) return;
		do {
			if (!socket_->read (line, false)) return;
			if (line.size() > 1) {
				mail.push_back (line.substr(0, line.size()-1));
#ifdef DEBUG
				g_print ("+");
#endif
			}
			else
				mail.push_back ("");
		} while (line != ".\r");
#ifdef DEBUG
		g_print("\n");
#endif
		mail.pop_back();
		parse (mail, MAIL_UNREAD);
	}
	

	// LOGOUT
	line = "QUIT\r\n";
	if (!socket_->write (line)) return;
	socket_->close();

	// Restore status
	status_ = saved_status;

	// Last check for mailbox status
	if ((unread_ == new_unread_) && (unread_.size() > 0))
		status_ = MAILBOX_OLD;

	unread_ = new_unread_;
	seen_ = new_seen_;
}
