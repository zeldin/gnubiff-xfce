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

#ifndef __APPLET_SYSTRAY_H__
#define __APPLET_SYSTRAY_H__

#include <gtk/gtk.h>

#include "eggtrayicon.h"
#include "ui-applet-gtk.h"


class AppletSystray : public AppletGtk {
 private:
	/// Icon in the system tray
	EggTrayIcon *trayicon_;

 public:
	// ========================================================================
	//  base
	// ========================================================================
	AppletSystray (class Biff *biff);
	~AppletSystray (void);

	// ========================================================================
	//  main
	// ========================================================================
	void show (std::string name = "dialog");
	void resize (guint width, guint height);

	// ========================================================================
	//  callbacks
	// ========================================================================
	static void signal_size_allocate (GtkWidget *widget,
									  GtkAllocation *allocation,
									  gpointer data);
};

#endif
