// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2005 Nicolas Rougier
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
// Author(s)     : Robert Sowada, Nicolas Rougier
// Short         : Mh protocol as used by Sylpheed
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#ifndef __MH_SYLPHEED_H__
#define __MH_SYLPHEED_H__

#include <glib.h>
#include <vector>
#include "mh_basic.h"

/**
 * Local mailbox for the mh protocol (as used by sylpheed).
 *
 * Note: The file ".sylpheed_mark" in which sylpheed saves information about
 * messages is at the moment not platform independent!
 */
class Mh_Sylpheed : public Mh_Basic {

protected:

public:
	// ========================================================================
	//  base
	// ========================================================================
	Mh_Sylpheed (class Biff *biff);
	Mh_Sylpheed (const Mailbox &other);
	~Mh_Sylpheed (void);

	// ========================================================================
	//  main
	// ========================================================================
	void get_messagenumbers (std::vector<guint> &msn,
							 gboolean empty = true) throw (local_err);
	std::string file_to_monitor (void);
};

#endif
