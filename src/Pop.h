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
#ifndef _POP_H
#define _POP_H

#ifdef HAVE_CONFIG_H
#   include "../config.h"
#endif
#include "Mailbox.h"


// ===================================================================
// = Some useful macros (as always) ==================================
// ===================================================================
#ifdef CHECK_SOCKET
#  undef CHECK_SOCKET
#endif
#define CHECK_SOCKET		if (_socket_status == SOCKET_STATUS_ERROR) {socket_close(); _status = MAILBOX_ERROR; return;}
#ifdef CHECK_SERVER_RESPONSE
# undef CHECK_SERVER_RESPONSE
#endif
#define CHECK_SERVER_RESPONSE	CHECK_SOCKET; if (line.find ("-ERR") == 0) {_socket_status = SOCKET_STATUS_ERROR; _status = MAILBOX_ERROR; return;}


class Pop : public Mailbox {

  // ===================================================================
  // - Public methods --------------------------------------------------
  // ===================================================================
 public:
	Pop (class Biff *owner);
	Pop (class Mailbox *other);
	~Pop (void);
	void get_status (void);
	void get_header (void);


  // ===================================================================
  // - Protected methods -----------------------------------------------
  // ===================================================================
 protected:
	virtual void connect (void) = 0;


  // ===================================================================
  // - Protected attributes --------------------------------------------
  // ===================================================================
 protected:
	std::vector<std::string> _saved;	// Saved UIDL list
};


#endif
