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

#ifndef __LOCAL_H__
#define __LOCAL_H__

#include "mailbox.h"
#ifdef HAVE_FAM_H
#	include <fam.h>
#endif
#include <glib.h>
#include <string>

#ifdef HAVE_LIBFAM
const gboolean use_fam = 1;
#else
const gboolean use_fam = 0;
#endif

#define LOCAL(x)					(static_cast<Local *>(x))

/**
 *  Base class for all local mailbox protocols.
 */
class Local : public Mailbox {

protected:
#ifdef HAVE_FAM_H
	// ========================================================================
	//  monitoring stuff (using FAM, File Alteration Monitor)
	// ========================================================================
	FAMConnection   fam_connection_;
	FAMRequest      fam_request_;
	FAMEvent        fam_event_;
#endif
	/**
	 *  This boolean indicates whether a FAM connection is being open or not.
	 *  It must only be read or changed when the fam_mutex_ is locked by the
	 *  thread.
	 */
	gboolean        fam_is_open_;
	/**
	 *  This mutex must be locked before calling any FAM function
	 *  (when not having the Mailbox::monitor_mutex_). To call
	 *  FAMOpen() or FAMClose() and change the value of
	 *  Local::fam_is_open_ the Mailbox::monitor_mutex_ must be
	 *  locked. For calling functions like FAMNextEvent() it is
	 *  sufficient to have the Mailbox::monitor_mutex_, so others with
	 *  this mutex can call functions like FAMCancelMonitor().
	 */
	GMutex          *fam_mutex_;

public:
	// ========================================================================
	//  base
	// ========================================================================
	Local (class Biff *biff);
	Local (const Mailbox &other);
	virtual ~Local (void);

	// ========================================================================
	//  exceptions
	// ========================================================================
	/** Generic exception for local mailboxes serving as a base for more
	 *  specific exceptions. */
	class local_err : public mailbox_err {};
	/// Exception for a problem with the file alteration monitor (FAM).
	class local_fam_err : public local_err {};
	/// Exception for a problem when opening or reading a file
	class local_file_err : public local_err {};
	/** This exception should be thrown if there is a problem when obtaining
	 *  information that is necessary to get and parse the messages (e.g. if
	 *  no message sequence numbers can be obtained). */
	class local_info_err : public local_err {};

	// ========================================================================
	//  main
	// ========================================================================
	void start (void);
	void stop (void);								// stop method
	virtual std::string file_to_monitor (void);
	void parse_single_message_file (const std::string &filename,
				const std::string uid = std::string ("")) throw (local_err);

	// ========================================================================
	//  file alteration monitor (FAM)
	// ========================================================================
	void fam_cancel_monitor (void);
	void fam_close (void);
	void fam_get_all_pending_events (void);
	void fam_monitoring (void) throw (local_err);
	void fam_start_monitoring (void);
};

#endif
