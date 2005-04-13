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

#ifndef __LOCAL_H__
#define __LOCAL_H__

#include "mailbox.h"
#include <fam.h>
#include <string>

#define LOCAL(x)					((Local *)(x))

/**
 *  Base class for all local mailbox protocols.
 */
class Local : public Mailbox {

protected:
	// ========================================================================
	//  monitoring stuff (using FAM, File Alteration Monitor)
	// ========================================================================
	FAMConnection   fam_connection_;
	FAMRequest      fam_request_;
	FAMEvent        fam_event_;

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
	void start (void);								// start method
	void fam_monitoring (void) throw (local_err);
	void stop (void);								// stop method
	virtual std::string file_to_monitor (void);
	void parse_single_message_file (const std::string &filename,
				const std::string uid = std::string ("")) throw (local_err);
};

#endif
