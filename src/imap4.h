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

#ifndef __IMAP4_H__
#define __IMAP4_H__

#include "mailbox.h"
#include "socket.h"

#define IMAP4(x)				((Imap4 *)(x))


class Imap4 : public Mailbox {

protected:
	class Socket *				socket_;		// socket to talk to server
	std::vector<int>			saved_;			// saved uidl's
	gboolean					idleable_;		// does server support the IDLE capability ?
	gboolean					idled_;			// Is the  server ucrrently idled

public:
	// ========================================================================
	//  base
	// ========================================================================	
	Imap4 (class Biff *owner);
	Imap4 (const Mailbox &other);
	~Imap4 (void);

	// ========================================================================
	//  main
	// ========================================================================	
	virtual void threaded_start (guint delay = 0);
	void start (void);
	void fetch (void);
	gint connect (void);
	void fetch_status (void);
	void fetch_header (void);
};

#endif
