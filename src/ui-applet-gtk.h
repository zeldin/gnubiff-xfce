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

#ifndef __APPLET_GTK_H__
#define __APPLET_GTK_H__

#include "ui-applet.h"


class AppletGtk : public Applet {

 public:
	/* base */
	AppletGtk (class Biff *biff);
	~AppletGtk (void);

	/* main */
	void update (void);
	gint create (void);
	void show (std::string name = "dialog");
	void tooltip_update (void);

	/* callbacks */
	gboolean on_button_press (GdkEventButton *event);
	void on_menu_command (void);
	void on_menu_mark (void);
	void on_menu_preferences (void);
	void on_menu_about (void);
	void on_menu_quit (void);
	void on_hide_about (void);
};

#endif
