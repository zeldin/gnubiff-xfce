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
	DIR *dir;
	struct stat file_stat;
	struct dirent *dent;
	int saved_status = status();
  
	// Build directory name
	gchar *base = g_path_get_basename (address().c_str());
	std::string directory;
	if (base == std::string("new"))
		directory = address ();
	else
	{
		gchar *tmp = g_build_filename (address().c_str(), "new", NULL);
		directory = std::string(tmp);
		g_free(tmp);
	}
	g_free(base);

	// Check for existence of a new mail directory
	if ((stat (directory.c_str(), &file_stat) != 0)||(!S_ISDIR(file_stat.st_mode))) {
		g_warning (_("Cannot find new mail directory (%s)"), directory.c_str());
		status (MAILBOX_ERROR);
		return;
	}

	// Try to open new mail directory
	if ((dir = opendir (directory.c_str())) == NULL) {
		g_warning (_("Cannot open new mail directory (%s)"), directory.c_str());
		status (MAILBOX_ERROR);
		return;
	}

	std::vector<std::string> mail;
	std::string line; 
	// Read new mails
	while ((dent = readdir(dir)) && (new_unread_.size() < (biff_->value_uint ("max_mail")))) {
		if (dent->d_name[0]=='.')
			continue;

		std::ifstream file;
		gchar *tmp=g_build_filename(directory.c_str(),dent->d_name,NULL);
		std::string filename(tmp);
		g_free(tmp);

		file.open (filename.c_str());
		if (file.is_open()) {
			while (!file.eof()) {
				std::string line;
				getline(file, line);
				mail.push_back (line);
			}
			parse (mail);
			mail.clear();
		}
		else
			g_warning (_("Cannot open %s."), filename.c_str());
		file.close();
	}
	closedir (dir);

	// Restore status
	status (saved_status);
}
