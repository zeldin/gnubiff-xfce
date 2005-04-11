// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2005 Nicolas Rougier
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

#include <errno.h>
#include <fstream>
#include <signal.h>
#include <sstream>
#include <unistd.h>

#include "biff.h"
#include "local.h"
#include "support.h"
#include "ui-applet.h"

// ========================================================================
//  base
// ========================================================================	
Local::Local (Biff *biff) : Mailbox (biff)
{
	fam_connection_.fd = 0;
}

Local::Local (const Mailbox &other) : Mailbox (other)
{
	fam_connection_.fd = 0;
}

Local::~Local (void)
{
}


// ========================================================================
//  main
// ========================================================================	
void
Local::start (void)
{
	// is there already someone watching this mailbox ?
	if (!g_mutex_trylock (monitor_mutex_))
		return;
	
	// at this point we need to explicitely call the get function since
	// monitoring will start from now on. Even if the mailbox was full,
	// no change appears yet, so we force it.
	start_checking ();
	gdk_threads_enter();
	biff_->applet()->update();
	gdk_threads_leave();

	// connection request to FAM (File Alteration Monitor)
	if (FAMOpen (&fam_connection_) < 0) {
		g_mutex_unlock (monitor_mutex_);
		return ;
	}

	// start monitoring
	gint status;
	std::string file = file_to_monitor ();
	if (g_file_test (file.c_str(), G_FILE_TEST_IS_DIR))
		status = FAMMonitorDirectory (&fam_connection_, file.c_str(),
									  &fam_request_, NULL);
	else
		status = FAMMonitorFile (&fam_connection_, file.c_str(),
								 &fam_request_, NULL);
	if (status < 0) {
		FAMClose (&fam_connection_);
		g_mutex_unlock (monitor_mutex_);
		return;
	}

	status = 1;
	while (status == 1) {
		status = FAMNextEvent (&fam_connection_, &fam_event_);
		if( status < 0 ) {
			if( errno == EINTR )
				break;
			FAMClose (&fam_connection_);
			g_mutex_unlock (monitor_mutex_);
			return ;
		}

		if ((fam_event_.code == FAMChanged)
			|| (fam_event_.code == FAMCreated)
			|| (fam_event_.code == FAMDeleted)) {
			start_checking ();
			gdk_threads_enter();
			biff_->applet()->update();
			gdk_threads_leave();
		}
		else if (fam_event_.code == FAMAcknowledge)
			break;
	}

	FAMClose (&fam_connection_);
	g_mutex_unlock (monitor_mutex_);

	// Ok, we got an error, just retry monitoring
	if (status != 1) {
#if DEBUG
		g_message ("[%d] FAM error, start fetch in %d second(s)", uin(),
				   delay());
#endif
		sleep (delay());
		start ();
	}
}

void Local::stop (void)
{
	Mailbox::stop ();
	if (FAMCONNECTION_GETFD (&fam_connection_)) 
		FAMCancelMonitor (&fam_connection_, &fam_request_);
}

/**
 *  Give the name of the file that shall be monitored by FAM.
 *
 *  Note: This may be different from the address when information about new
 *  messages is stored in a separate file.
 *
 *  @return    Name of the file to be monitored.
 */
std::string 
Local::file_to_monitor (void)
{
	return address ();
}

/**
 *  Read und parse a file that contains a single message.
 *
 *  @param  filename  Name of the file
 *  @param  uid       Unique identifier of the message (if known) or the empty
 *                    string (the default is the empty string)
 */
void 
Local::parse_single_message_file (const std::string &filename,
								  const std::string uid)
{
	std::ifstream file;
	std::vector<std::string> mail;
	std::string line;
	guint max_cnt = 1 + biff_->value_uint ("min_body_lines");


	// Read message header and first lines of message's body
	file.open (filename.c_str());
	if (!file.is_open()) {
		g_warning (_("Cannot open %s."), filename.c_str());
		return;
	}

	// Read header and first lines of message
	gboolean header = true;
	guint cnt = max_cnt;
	while ((!file.eof ()) && (cnt > 0)) {
		getline(file, line);
		// End of header?
		if ((line.size() == 0) && header)
			header = false;
		// Store line
		if (cnt > 0) {
			cnt--;
			mail.push_back (line);
		}
	}
	file.close();

	// Parse message
	parse (mail, uid);
}
