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

#ifndef __POP_H__
#define __POP_H__

#include "mailbox.h"


class Pop : public Mailbox {

protected:
	class Socket *	 			socket_;		// socket to talk to server
	std::vector<std::string> 	saved_;			// saved uidl's

public:
	/* base */
	Pop (class Biff *biff);
	Pop (const Mailbox &other);
	virtual ~Pop (void);

	/* main */
	virtual int connect (void) = 0;

	/* mailbox inherited methods */
	virtual void get_status (void);
	virtual void get_header (void);
};

#endif
