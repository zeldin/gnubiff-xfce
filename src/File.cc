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
#include "File.h"


// ================================================================================
//  Constructors & Destructors
// --------------------------------------------------------------------------------
//  
// ================================================================================
File::File (Biff *owner) : Mailbox (owner)
{
	_protocol = PROTOCOL_FILE;
	bzero(&_last_stat, sizeof(_last_stat));
}

File::File (Mailbox *other) : Mailbox (other)
{
	_protocol = PROTOCOL_FILE;
	bzero(&_last_stat, sizeof(_last_stat));
}

File::~File (void)
{
}


// ================================================================================
//  Try to quickly get mailbox status
// --------------------------------------------------------------------------------
//
// ================================================================================
void File::get_status (void) {
	struct stat file_stat;
	// To get the status of a mail file we look at modification time and
	// compare it with the saved one.  This methods is not 100% error
	// free because a mail client could modify the file in some way and
	// there would be no new mail at all. In this case, gnubiff would
	// report new mail while there aren't. This is not a big deal since
	// the fetch function will try to collect new mail and will realize
	// there aren't and modify status consequently. It will only slow
	// gnubiff a bit.

	_status = MAILBOX_ERROR;

	// Try to get stats on mailbox
	if (stat (_address.c_str(), &file_stat) != 0)
		return;

	//
	// The following rules borrowed from xbiff are used:
	//
	// 1) if no mailbox or empty (zero-sized) mailbox mark EMPTY
	// 2) if read after most recent write mark EMPTY
	// 3) if same size as last time no change
	// 4) if change in size mark NEW
	//
	if (file_stat.st_size == 0)
		_status = MAILBOX_EMPTY;
	else
		if (file_stat.st_atime > file_stat.st_mtime)
			_status = MAILBOX_EMPTY;
		else if (file_stat.st_size != _last_stat.st_size)
			_status = MAILBOX_NEW;


	memcpy(&_last_stat, &file_stat, sizeof(struct stat));
}


// ================================================================================
//  Get headers
// --------------------------------------------------------------------------------
//  
// ================================================================================
void File::get_header (void) {
	struct stat file_stat;
	struct utimbuf timbuf;
	int saved_status = _status;
  
	// Status will be restored in the end if no problem occured
	_status = MAILBOX_CHECKING;

	// First we save access time of the mailfile to be able to reset it
	// before exiting this function because some mail clients (e.g. mutt)
	// rely on this access time to perform some operations.
	if (stat (_address.c_str(), &file_stat) != 0) {
		_status = MAILBOX_ERROR;
		return;
	}
	timbuf.actime = file_stat.st_atime;
	timbuf.modtime = file_stat.st_mtime;


	// Open mailbox for reading
	std::ifstream file;
	file.open (_address.c_str());
	if (!file.is_open()) {
		_status = MAILBOX_ERROR;
		return;
	}

	// Try to get mails

	// We should restrict our collect to _owner->_max_header_display and try
	// to get unread mail in priority. Nonetheless, with this protocol,
	// we have no way of geeting only unread mail without parsing the
	// whole mailfile that can be pretty big. So instead of doing that,
	// we read the file sequentially until we get enough mails knowing
	// that new mail should be at the beginning of the file.
	//
	std::vector<header> old_unread = _unread;
	_unread.clear();
	_seen.clear();
	std::vector<std::string> mail;
	std::string line; 
	getline(file, line);
	mail.push_back (line);
	while (!file.eof() && (_unread.size() < (unsigned int)(_owner->_max_collected_mail))) {
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
	utime (_address.c_str(), &timbuf);

	// Restore status
	_status = saved_status;

	if ((_unread == old_unread) && (_unread.size() > 0))
		_status = MAILBOX_OLD;

	// Last check in case mailbox empty (because pine leaves one internal data mail)
	if (_unread.size() == 0)
		_status = MAILBOX_EMPTY;  
}
