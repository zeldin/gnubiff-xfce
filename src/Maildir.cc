/* gnubiff -- a mail notification program
 * Copyright (c) 2000-2004 Nicolas Rougier
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * This file is part of gnubiff.
 */
#include <sstream>
#include <sys/stat.h>
#include <utime.h>
#include <dirent.h>
#include <time.h>
#include "Maildir.h"


// ================================================================================
//  Constructors & Destructors
// --------------------------------------------------------------------------------
//  
// ================================================================================
Maildir::Maildir (Biff *owner) : Mailbox (owner)
{
	_protocol = PROTOCOL_MAILDIR;
	_folder = "new";
	_last_mtime = 0;
}

Maildir::Maildir (Mailbox *other) : Mailbox (other)
{
	_protocol = PROTOCOL_MAILDIR;
	_folder = "new";
	_last_mtime = 0;
}

Maildir::~Maildir (void)
{
}


// ================================================================================
//  Try to quickly get mailbox status
// --------------------------------------------------------------------------------
//
// ================================================================================
void Maildir::get_status (void) {
	struct stat file_stat;
	DIR *dir;
	struct dirent *dent;
	int dirsize=0;

	// Default behavior
	_status = MAILBOX_CHECKING;

	std::string directory = _address + std::string("/") + _folder;
	// Check for existence of a new mail directory
	if ((stat (directory.c_str(), &file_stat) != 0)||(!S_ISDIR(file_stat.st_mode))) {
		g_warning (_("Cannot find new mail directory (%s)"), directory.c_str());
		_status = MAILBOX_ERROR;
		return;
	}

	// Try to open new mail directory
	if ((dir = opendir (directory.c_str())) == NULL) {
		g_warning (_("Cannot open new mail directory (%s)"), directory.c_str());
		_status = MAILBOX_ERROR;
		return;
	}

	// Read number of entries (but '.'  and '..')
	while ((dent = readdir(dir)))
		if (dent->d_name[0] != '.')
			dirsize++;
	closedir (dir); 
   
	// No entry  = no new mail
	if (dirsize == 0)
		_status = MAILBOX_EMPTY;
	else {
		// New mail directory has not been modified
		if (file_stat.st_mtime == _last_mtime)
			_status = MAILBOX_OLD;
		// New mail directory has been modified
		else
			_status = MAILBOX_NEW;
	}

	// Save new modification time for next time
	_last_mtime = file_stat.st_mtime;
}


// ================================================================================
//  Get headers
// --------------------------------------------------------------------------------
//  
// ================================================================================
void Maildir::get_header (void) {
	DIR *dir;
	struct stat file_stat;
	struct dirent *dent;
	int saved_status = _status;
  
	// Status will be restored in the end if no problem occured
	_status = MAILBOX_CHECKING;

	std::string directory = _address + std::string("/") + _folder;
	// Check for existence of a new mail directory
	if ((stat (directory.c_str(), &file_stat) != 0)||(!S_ISDIR(file_stat.st_mode))) {
		g_warning (_("Cannot find new mail directory (%s)"), directory.c_str());
		_status = MAILBOX_ERROR;
		return;
	}

	// Try to open new mail directory
	if ((dir = opendir (directory.c_str())) == NULL) {
		g_warning (_("Cannot open new mail directory (%s)"), directory.c_str());
		_status = MAILBOX_ERROR;
		return;
	}

	std::vector<header> old_unread = _unread;
	_unread.clear();
	_seen.clear();
	std::vector<std::string> mail;
	std::string line; 
	// Read new mails
	while ((dent = readdir(dir)) && (_unread.size() < (unsigned int)(_owner->_max_collected_mail))) {
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
	_status = saved_status;

	if ((_unread == old_unread) && (_unread.size() > 0))
		_status = MAILBOX_OLD;
}
