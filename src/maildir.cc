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

#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <utime.h>
#include <dirent.h>
#include "maildir.h"
#include "nls.h"


Maildir::Maildir (Biff *biff) : Mailbox (biff)
{
	protocol_ = PROTOCOL_MAILDIR;
	last_mtime_ = 0;
}

Maildir::Maildir (const Mailbox &other) : Mailbox (other)
{
	protocol_ = PROTOCOL_MAILDIR;
	last_mtime_ = 0;
}

Maildir::~Maildir (void)
{
}

void
Maildir::get_status (void)
{
	struct stat file_stat;
	DIR *dir;
	struct dirent *dent;
	int dirsize=0;

	// Default behavior
	status_ = MAILBOX_CHECKING;

	// Build directory name
	std::string directory;
	if (location_[location_.size()-1] == '/')
		directory = location_.substr (0, location_.size()-1);
	else
		directory = location_;
	std::string lastdir = directory.substr (directory.find_last_of ('/'));
	if (lastdir != "/new")
		directory += "/new";


	// Check for existence of a new mail directory
	if ((stat (directory.c_str(), &file_stat) != 0)||(!S_ISDIR(file_stat.st_mode))) {
		g_warning (_("Cannot find new mail directory (%s)"), directory.c_str());
		status_ = MAILBOX_ERROR;
		return;
	}

	// Try to open new mail directory
	if ((dir = opendir (directory.c_str())) == NULL) {
		g_warning (_("Cannot open new mail directory (%s)"), directory.c_str());
		status_ = MAILBOX_ERROR;
		return;
	}

	// Read number of entries (but '.'  and '..')
	while ((dent = readdir(dir)))
		if (dent->d_name[0] != '.')
			dirsize++;
	closedir (dir); 
   
	// No entry  = no new mail
	if (dirsize == 0)
		status_ = MAILBOX_EMPTY;
	else {
		// New mail directory has not been modified
		if (file_stat.st_mtime == last_mtime_)
			status_ = MAILBOX_OLD;
		// New mail directory has been modified
		else
			status_ = MAILBOX_NEW;
	}

	// Save new modification time for next time
	last_mtime_ = file_stat.st_mtime;
}

void
Maildir::get_header (void)
{
	DIR *dir;
	struct stat file_stat;
	struct dirent *dent;
	int saved_status = status_;
  
	// Status will be restored in the end if no problem occured
	status_ = MAILBOX_CHECKING;

	// Build directory name
	std::string directory;
	if (location_[location_.size()-1] == '/')
		directory = location_.substr (0, location_.size()-1);
	else
		directory = location_;
	std::string lastdir = directory.substr (directory.find_last_of ('/'));
	if (lastdir != "/new")
		directory += "/new";

	// Check for existence of a new mail directory
	if ((stat (directory.c_str(), &file_stat) != 0)||(!S_ISDIR(file_stat.st_mode))) {
		g_warning (_("Cannot find new mail directory (%s)"), directory.c_str());
		status_ = MAILBOX_ERROR;
		return;
	}

	// Try to open new mail directory
	if ((dir = opendir (directory.c_str())) == NULL) {
		g_warning (_("Cannot open new mail directory (%s)"), directory.c_str());
		status_ = MAILBOX_ERROR;
		return;
	}

	new_unread_.clear();
	new_seen_.clear();
	std::vector<std::string> mail;
	std::string line; 
	// Read new mails
	while ((dent = readdir(dir)) && (new_unread_.size() < (unsigned int)(biff_->max_mail_))) {
		if (dent->d_name[0]=='.')
			continue;
		std::ifstream file;
		std::string filename = directory + std::string("/") + std::string (dent->d_name);
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
		else {
			g_warning (_("Cannot open %s."), filename.c_str());
		}
		file.close();
	}
	closedir (dir);

	// Restore status
	status_ = saved_status;

	if ((unread_ == new_unread_) && (new_unread_.size() > 0))
		status_ = MAILBOX_OLD;

	unread_ = new_unread_;
	seen_ = new_seen_;
}
