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

File::File (Biff *biff) : Mailbox (biff)
{
	protocol_ = PROTOCOL_FILE;
	bzero (&last_stat_, sizeof(last_stat_));
}

File::File (const Mailbox &other) : Mailbox (other)
{
	protocol_ = PROTOCOL_FILE;
	bzero (&last_stat_, sizeof (last_stat_));
}

File::~File (void)
{
}

void
File::get_status (void)
{
	struct stat file_stat;
	// To get the status of a mail file we look at modification time and
	// compare it with the saved one.  This methods is not 100% error
	// free because a mail client could modify the file in some way and
	// there would be no new mail at all. In this case, gnubiff would
	// report new mail while there aren't. This is not a big deal since
	// the fetch function will try to collect new mail and will realize
	// there aren't and modify status consequently. It will only slow
	// gnubiff a bit.

	// Try to get stats on mailbox
	if (stat (location_.c_str(), &file_stat) != 0) {
		status_ = MAILBOX_ERROR;
		return;
	}

	//
	// The following rules borrowed from xbiff are used:
	//
	// 1) if no mailbox or empty (zero-sized) mailbox mark EMPTY
	// 2) if read after most recent write mark EMPTY
	// 3) if same size as last time no change
	// 4) if change in size mark NEW
	//
	if (file_stat.st_size == 0)
		status_ = MAILBOX_EMPTY;
	else
		if (file_stat.st_atime > file_stat.st_mtime)
			status_ = MAILBOX_EMPTY;
		else if (file_stat.st_size != last_stat_.st_size)
			status_ = MAILBOX_NEW;


	memcpy (&last_stat_, &file_stat, sizeof (struct stat));
}


void File::get_header (void)
{
	struct stat file_stat;
	struct utimbuf timbuf;
	int saved_status = status_;
  
	// Status will be restored in the end if no problem occured
	status_ = MAILBOX_CHECKING;

	// First we save access time of the mailfile to be able to reset it
	// before exiting this function because some mail clients (e.g. mutt)
	// rely on this access time to perform some operations.
	if (stat (location_.c_str(), &file_stat) != 0) {
		status_ = MAILBOX_ERROR;
		return;
	}
	timbuf.actime = file_stat.st_atime;
	timbuf.modtime = file_stat.st_mtime;


	// Open mailbox for reading
	std::ifstream file;
	file.open (location_.c_str());
	if (!file.is_open()) {
		status_ = MAILBOX_ERROR;
		return;
	}

	// Try to get mails

	// We should restrict our collect to biff_->_max_header_display and try
	// to get unread mail in priority. Nonetheless, with this protocol,
	// we have no way of geeting only unread mail without parsing the
	// whole mailfile that can be pretty big. So instead of doing that,
	// we read the file sequentially until we get enough mails knowing
	// that new mail should be at the beginning of the file.
	//
	new_unread_.clear();
	new_seen_.clear();
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
	utime (location_.c_str(), &timbuf);

	// Restore status
	status_ = saved_status;

	if ((unread_ == new_unread_) && (new_unread_.size() > 0))
		status_ = MAILBOX_OLD;

	// Last check in case mailbox empty (because pine leaves one internal data mail)
	if (new_unread_.size() == 0)
		status_ = MAILBOX_EMPTY;

	unread_ = new_unread_;
	seen_ = new_seen_;	
}
