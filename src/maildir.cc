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

#include "support.h"

#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <utime.h>
#include <dirent.h>

#include "maildir.h"


// ========================================================================
//  base
// ========================================================================	
Maildir::Maildir (Biff *biff) : Local (biff)
{
	value ("protocol", PROTOCOL_MAILDIR);
}

Maildir::Maildir (const Mailbox &other) : Local (other)
{
	value ("protocol", PROTOCOL_MAILDIR);
}

Maildir::~Maildir (void)
{
}

// ========================================================================
//  main
// ========================================================================	
void
Maildir::fetch (void)
{
	int saved_status = status();

	// Check for existence of a new mail directory
	if (!g_file_test (address().c_str(), G_FILE_TEST_IS_DIR)) {
		g_warning(_("Cannot find new mail directory (%s)"), address().c_str());
		status (MAILBOX_ERROR);
		return;
	}

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

	std::vector<std::string> mail;
	std::string line;
	guint max_cnt = 1 + biff_->value_uint ("min_body_lines");

	const gchar *d_name;
	// Read new mails
	while ((d_name = g_dir_read_name (gdir)) && (new_unread_.size() < maxnum)){
		// Filenames that begin with '.' are no messages in maildir protocol
		if (d_name[0] == '.')
			continue;

		// If the mail is already known, we don't need to parse it
		std::string uid = std::string (d_name);
		if (new_mail (uid))
			continue;

		std::ifstream file;
		gchar *tmp = g_build_filename (address().c_str(), d_name, NULL);
		std::string filename(tmp);
		g_free(tmp);

		// Read message header and first lines of message's body
		file.open (filename.c_str());
		if (file.is_open()) {
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
			parse (mail, uid);
			mail.clear();
		}
		else
			g_warning (_("Cannot open %s."), filename.c_str());
		file.close();
	}

	// Close directory
	g_dir_close (gdir);

	// Restore status
	status (saved_status);
}
