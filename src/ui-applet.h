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

#ifndef __APPLET_H__
#define __APPLET_H__

#include "gui.h"
#include "biff.h"


class Applet : public GUI {

protected:
	class Biff *		biff_;
	GMutex *			process_mutex_;
	GMutex *			update_mutex_;
	gboolean			force_popup_;

public:
	// ========================================================================
	//  base
	// ========================================================================	
	Applet (class Biff *biff,
			std::string filename);
	virtual ~Applet (void);

	// ========================================================================
	//  main
	// ========================================================================	
	void start (guint delay=0);						// start monitoring
	void stop  (void);								// stop monitoring
	virtual void update (gboolean no_popup = false);// update applet
	virtual void dock (GtkWidget *applet) {};		// dock applet

	guint       unread_markup (std::string &text);	// build unread markup string
	std::string tooltip_text (void);				// build tooltip text

};

#endif
