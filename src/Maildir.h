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
#ifndef _MAILDIR_H
#define _MAILDIR_H

#ifdef HAVE_CONFIG_H
#   include "../config.h"
#endif
#include "Mailbox.h"


class Maildir : public Mailbox {

	//  Public methods
	// ===================================================================
 public:
	Maildir (class Biff *owner);
	Maildir (class Mailbox *other);
	~Maildir (void);

	void get_status (void);
	void get_header (void);

	//  Private features
	// ===================================================================
 private:
	time_t _last_mtime;			// mtime at last check
};

#endif
