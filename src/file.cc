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
#include <utime.h>

#include "file.h"

// ========================================================================
//  base
// ========================================================================	
File::File (Biff *biff) : Local (biff)
{
	protocol_ = PROTOCOL_FILE;
}

File::File (const Mailbox &other) : Local (other)
{
	protocol_ = PROTOCOL_FILE;
}

File::~File (void)
{
}

// ========================================================================
//  main
// ========================================================================	
void File::fetch (void)
{
	struct stat file_stat;
	struct utimbuf timbuf;
  
	// Status will be restored in the end if no problem occured
	status_ = MAILBOX_CHECK;

	// First we save access time of the mailfile to be able to reset it
	// before exiting this function because some mail clients (e.g. mutt)
	// rely on this access time to perform some operations.
	if (stat (address_.c_str(), &file_stat) != 0) {
		status_ = MAILBOX_ERROR;
		return;
	}
	timbuf.actime = file_stat.st_atime;
	timbuf.modtime = file_stat.st_mtime;


	// Open mailbox for reading
	std::ifstream file;
	file.open (address_.c_str());
	if (!file.is_open()) {
		status_ = MAILBOX_ERROR;
		return;
	}

	new_unread_.clear ();
	new_seen_.clear ();
	std::vector<std::string> mail;
	std::string line; 
	getline(file, line);
	mail.push_back (line);
	while (!file.eof() && (new_unread_.size() < (unsigned int)(biff_->max_mail_))) {
		getline(file, line);
		// Here we look for a "From " at a beginning of a line indicating
		// a new mail header. We then parse previous mail, reset mail
		// vector, store new line in it and go on reading file.
		if (line.find ("From ", 0) == 0) {
			parse (mail);
			mail.clear();
		}
		mail.push_back (line);
	};
	// Do not forget to parse the last one that cannot relies on "From "
	// from the next mail
	if (mail.size() > 1)
		parse (mail);

	// Close mailbox
	file.close ();
 
	// Restore acces and modification time
	utime (address_.c_str(), &timbuf);

	if ((unread_ == new_unread_) && (new_unread_.size() > 0))
		status_ = MAILBOX_OLD;
	else if (new_unread_.size() > 0)
		status_ = MAILBOX_NEW;
	else if (new_unread_.size() == 0)
		status_ = MAILBOX_EMPTY;
	else
		status_ = MAILBOX_ERROR;

	unread_ = new_unread_;
	seen_ = new_seen_;
}
