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

#include <errno.h>
#include <fstream>
#include <signal.h>
#include <sstream>
#include <sys/select.h>
#include <unistd.h>

#include "biff.h"
#include "local.h"
#include "support.h"
#include "ui-applet.h"

// ========================================================================
//  base
// ========================================================================	
/**
 * Constructor. The local mailbox is created from scratch.
 *
 * @param biff Pointer to the instance of Gnubiff.
 */
Local::Local (Biff *biff) : Mailbox (biff)
{
	fam_mutex_ = g_mutex_new();
	fam_is_open_ = false;
}

/**
 * Constructor. The local mailbox is created by taking the attributes of
 * the existing mailbox {\em other}.
 *
 * @param other Mailbox from which the attributes are taken.
 */
Local::Local (const Mailbox &other) : Mailbox (other)
{
	fam_mutex_ = g_mutex_new();
	fam_is_open_ = false;
}

/// Destructor
Local::~Local (void)
{
	// Close FAM connection and free its mutex
	fam_close ();
	g_mutex_free (fam_mutex_);
}

// ========================================================================
//  main
// ========================================================================	

/**
 *  Monitor the files that need to be monitored by this mailbox via
 *  polling. The thread calling this function must have been locked
 *  Local::monitor_mutex_.
 */
void 
Local::start (void)
{
	// Is there already someone watching this mailbox?
	if (!g_mutex_trylock (monitor_mutex_))
		return;	

	if (use_fam && value_bool ("local_fam_enable"))
		fam_start_monitoring ();
	else {
		try {
			start_checking ();
			gdk_threads_enter();
			biff_->applet()->update();
			gdk_threads_leave();
		}
		catch (local_err &err) {
			// Catch all errors that are un-recoverable
#if DEBUG
			g_warning ("[%d] Local mailbox exception: %s", uin(), err.what());
#endif
			set_status_mailbox_error ();
		}
	}

	g_mutex_unlock (monitor_mutex_);

	// If we are polling, there must be another check after the delay time
	if (!use_fam || value_bool ("local_fam_enable") == false)
		threaded_start (delay ());
}

void 
Local::stop (void)
{
	// Do the usual stopping things
	Mailbox::stop ();

	// Cancel the FAM monitor
	fam_cancel_monitor ();
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
 *  @exception local_file_err
 *                    This exception is thrown if the file could not be opened.
 */
void 
Local::parse_single_message_file (const std::string &filename,
								  const std::string uid) throw (local_err)
{
	std::ifstream file;
	std::vector<std::string> mail;
	std::string line;
	guint max_cnt = 1 + biff_->value_uint ("min_body_lines");

	// Read message header and first lines of message's body
	file.open (filename.c_str());
	if (!file.is_open()) {
		g_warning (_("Cannot open %s."), filename.c_str());
		throw local_file_err();
	}

	// Read header and first lines of message
	gboolean header = true;
	guint cnt = max_cnt;
	getline (file, line);
	while ((!file.eof ()) && (cnt > 0)) {
		// End of header?
		if ((line.size() == 0) && header)
			header = false;
		// Store line
		if (!header)
			cnt--;
		mail.push_back (line);

		// Read next line
		getline(file, line);
	}
	file.close ();

	// Parse message
	parse (mail, uid);
}

// ========================================================================
//  file alteration monitor (FAM)
// ========================================================================

void 
Local::fam_cancel_monitor (void)
{
#ifdef HAVE_LIBFAM
	g_mutex_lock (fam_mutex_);
	if (fam_is_open_) {
		FAMCancelMonitor (&fam_connection_, &fam_request_);
		fam_is_open_ = false;
	}
	g_mutex_unlock (fam_mutex_);
#endif
}

/**
 *  Start monitoring the files that need to be monitored by this
 *  mailbox via FAM. If the FAM connection terminates because of an
 *  error, it is started again. The thread calling this function must
 *  have been locked Local::monitor_mutex_.
 */
void 
Local::fam_start_monitoring (void)
{
#ifdef HAVE_LIBFAM
	gboolean keep_monitoring = true;

	while (keep_monitoring) {
		// Start FAM monitoring
		try {
			fam_monitoring ();
			keep_monitoring = false;
		}
		catch (local_err &err) {
			// Catch all errors that are un-recoverable
#if DEBUG
			g_warning ("[%d] Local mailbox exception: %s", uin(), err.what());
			g_message ("[%d] Start fetch in %d second(s)", uin(), delay());
#endif
			status (MAILBOX_ERROR);
			unread_.clear ();
			seen_.clear ();

			// If we have a fam connection then close it
			fam_close ();

			// Wait the delay time before monitoring again
			sleep (delay ());
		}
	}
#endif
}

/**
 *  Close the FAM connection if it's open.
 */
void 
Local::fam_close (void)
{
#ifdef HAVE_LIBFAM
	g_mutex_lock (fam_mutex_);
	if (fam_is_open_) {
		FAMClose (&fam_connection_);
		fam_is_open_ = false;
	}
	g_mutex_unlock (fam_mutex_);
#endif
}


/**
 * Get all pending FAM events.
 *
 * Note: Depending on how this function
 * was called this call may result in some new messages not being
 * noticed because of race conditions.
 */

void 
Local::fam_get_all_pending_events (void)
{
#ifdef HAVE_LIBFAM
	g_mutex_lock (fam_mutex_);
	if (fam_is_open_)
		while (FAMPending (&fam_connection_))
			if (FAMNextEvent (&fam_connection_, &fam_event_) < 0)
					break;
	g_mutex_unlock (fam_mutex_);
#endif
}

/**
 *  Monitor the files that need to be monitored by this mailbox via
 *  FAM. The thread calling this function must have been locked
 *  Local::monitor_mutex_.
 *
 *  @exception local_fam_err
 *                     This exception is thrown when there is a problem with
 *                     FAM (File Alteration Monitor).
 */
void 
Local::fam_monitoring (void) throw (local_err)
{
#ifdef HAVE_LIBFAM
	gint status = 0;

	// Connection request to FAM (File Alteration Monitor)
	g_mutex_lock (fam_mutex_);
	status = FAMOpen (&fam_connection_);
	if (status < 0) {
		g_mutex_unlock (fam_mutex_);
		throw local_fam_err();
	}
	fam_is_open_ = true;

	// Start monitoring
	std::string file = file_to_monitor ();
	if (g_file_test (file.c_str(), G_FILE_TEST_IS_DIR))
		status = FAMMonitorDirectory (&fam_connection_, file.c_str(),
									  &fam_request_, NULL);
	else
		status = FAMMonitorFile (&fam_connection_, file.c_str(),
								 &fam_request_, NULL);

	// Initialize the structures needed by the select() command
	fd_set readfds;
	FD_ZERO (&readfds);
	FD_SET (FAMCONNECTION_GETFD (&fam_connection_), &readfds);

	// Release FAM lock
	g_mutex_unlock (fam_mutex_);

	if (status < 0) throw local_fam_err();

	// At this point we need to explicitely call the get function since
	// monitoring will start from now on. Even if the mailbox was full,
	// no change appears yet, so we force it.
	start_checking ();
	gdk_threads_enter();
	biff_->applet()->update();
	gdk_threads_leave();

	// Wait for and handle FAM events
	status = 1;
	while (status == 1) {
		// Wait for next event
		if (select (FAMCONNECTION_GETFD (&fam_connection_) + 1, &readfds, NULL,
					NULL, NULL) < 0) {
			if (errno==EINTR)
				break;
			else throw local_fam_err();
		}

		// Get the next event
		status = FAMNextEvent (&fam_connection_, &fam_event_);

		if ((status < 0 ) && (errno == EINTR))
			break;
		if (status < 0) throw local_fam_err();

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

	// Close FAM connection
	fam_close ();

	// Ok, we got an error, just retry monitoring
	if (status != 1) throw local_fam_err();
#endif
}
