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
// Author(s)     : Nicolas Rougier, Robert Sowada
// Short         : Base class for all local protocols similar to mh
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#include <fstream>
#include <sstream>
#include "mh_basic.h"

// ========================================================================
//  base
// ========================================================================	
/**
 * Constructor. The local mailbox for the mh_basic protocol is created from
 * scratch.
 *
 * @param biff Pointer to the instance of Gnubiff.
 */
Mh_Basic::Mh_Basic (Biff *biff) : Local (biff)
{
	value ("protocol", PROTOCOL_MH_BASIC);
}

/**
 * Constructor. The local mailbox for the mh_basic protocol is created by
 * taking the attributes of the existing mailbox {\em other}.
 *
 * @param other Mailbox from which the attributes are taken.
 */
Mh_Basic::Mh_Basic (const Mailbox &other) : Local (other)
{
	value ("protocol", PROTOCOL_MH_BASIC);
}

/// Destructor
Mh_Basic::~Mh_Basic (void)
{
}

// ========================================================================
//  main
// ========================================================================	

/**
 *  Get new messages.
 */
void 
Mh_Basic::fetch (void)
{
	// Get message numbers of (hopefully) unread mails
	std::vector<guint> msn;
	if (!get_messagenumbers(msn)) {
		status (MAILBOX_ERROR);
		return;
	}

	// Get maximum number of mails to catch
	guint maxnum = INT_MAX;
	if (biff_->value_bool ("use_max_mail"))
		maxnum = biff_->value_uint ("max_mail");

	for (guint i=0; (i<msn.size()) && (new_unread_.size() < maxnum); i++) {
		std::vector<std::string> mail;
		std::string line;
		std::ifstream file;

		// Create filename for message
		std::stringstream ss;
		ss << msn[i];
		std::string filename = add_file_to_path (address(), ss.str());

		// Open, read and parse message
		file.open (filename.c_str());
		if (file.is_open()) {
			while (!file.eof()) {
				getline(file, line);
				mail.push_back(line);
			}
			parse (mail);
			file.close();
		}
		else
			g_warning (_("Cannot open %s."), filename.c_str());
	}
}

/**
 *  Get message numbers of the mails to be parsed. In the mh_basic protocol we
 *  do not have any information about unread mails. So we have to return all
 *  message numbers that are present in the directory.
 *
 *  @param  msn    Reference to a vector in which the message numbers are
 *                 returned
 *  @param  empty  Whether the vector shall be emptied before obtaining the
 *                 message numbers (the default is true)
 *  @return        Boolean indicating success
 */
gboolean 
Mh_Basic::get_messagenumbers (std::vector<guint> &msn, gboolean empty)
{
	// Empty the vector if wished for
	if (empty)
		msn.clear ();

	// Try to open the directory
	GDir *gdir = g_dir_open (address().c_str(), 0, NULL);
	if (gdir == NULL) {
		g_warning(_("Cannot open new mail directory (%s)"), address().c_str());
		return false;
	}

	// Read filenames from directory
	const gchar *d_name;
	gchar c;
	while ((d_name = g_dir_read_name (gdir)) != NULL) {
		// Convert filename to message number
		guint pos = 0, num = 0;
		while (((c = d_name[pos++]) != 0) && (g_ascii_isdigit (c)))
			num = 10*num + (c - '0');

		// Valid message number?
		if ((num > 0) && (c == 0))
			msn.push_back (num);
	}

	// Close directory
	g_dir_close (gdir);

	return true;
}
