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

#include "maildir.h"
#include "support.h"


// ========================================================================
//  base
// ========================================================================	
/**
 * Constructor. The local mailbox for the maildir protocol is created from
 * scratch.
 *
 * @param biff Pointer to the instance of Gnubiff.
 */
Maildir::Maildir (Biff *biff) : Local (biff)
{
	value ("protocol", PROTOCOL_MAILDIR);
}

/**
 * Constructor. The local mailbox for the maildir protocol is created by
 * taking the attributes of the existing mailbox {\em other}.
 *
 * @param other Mailbox from which the attributes are taken.
 */
Maildir::Maildir (const Mailbox &other) : Local (other)
{
	value ("protocol", PROTOCOL_MAILDIR);
}

/// Destructor
Maildir::~Maildir (void)
{
}

// ========================================================================
//  main
// ========================================================================	
void
Maildir::fetch (void)
{
	// Try to open new mail directory
	GDir *gdir = g_dir_open (address().c_str(), 0, NULL);
	if (gdir == NULL) {
		g_warning(_("Cannot open new mail directory (%s)"), address().c_str());
		status (MAILBOX_ERROR);
		return;
	}

	// Get maximum number of mails to catch
	guint maxnum = INT_MAX;
	if (biff_->value_bool ("use_max_mail"))
		maxnum = biff_->value_uint ("max_mail");

	const gchar *d_name;
	// Read new mails
	while ((d_name = g_dir_read_name (gdir)) && (new_unread_.size() < maxnum)){
		// Filenames that begin with '.' are not messages in maildir protocol
		if (d_name[0] == '.')
			continue;

		// If the mail is already known, we don't need to parse it
		std::string uid = std::string (d_name);
		if (new_mail (uid))
			continue;

		// Read and parse file
		parse_single_message_file (add_file_to_path (address(), d_name), uid);
	}

	// Close directory
	g_dir_close (gdir);
}
