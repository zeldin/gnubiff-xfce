// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2007 Nicolas Rougier, 2004-2007 Robert Sowada
//
// This program is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ========================================================================
//
// File          : $RCSfile$
// Revision      : $Revision$
// Revision date : $Date$
// Author(s)     : Nicolas Rougier, Robert Sowada
// Short         : 
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#ifndef __APPLET_XFCE4_H__
#define __APPLET_XFCE4_H__

#include <libxfce4panel/xfce-panel-plugin.h>
#include "ui-applet-gui.h"


class AppletXfce4 : public AppletGUI {

protected:
	// Pointer to the XfcePanelPlugin of the applet
	XfcePanelPlugin *plugin_;
	GtkWidget *alignment_;

public:
	// ========================================================================
	//  base
	// ========================================================================
	AppletXfce4 (class Biff *biff);
	~AppletXfce4 (void);

	// ========================================================================
	//  tools
	// ========================================================================
	gboolean get_orientation (GtkOrientation &orient);

	// ========================================================================
	//  main
	// ========================================================================
	void dock (XfcePanelPlugin *plugin);
	gboolean update (gboolean init = false);
	void show (std::string name = "dialog");
	void hide (std::string name = "dialog");
	gboolean calculate_size (gint size);

	// ========================================================================
	//  callbacks
	// ========================================================================
	gboolean on_button_press (GdkEventButton *event);
	gboolean on_size_changed (XfcePanelPlugin *plugin, gint size);
	static void gnubiff_applet_construct (XfcePanelPlugin *plugin);
	static gboolean gnubiff_applet_preinit (gint argc, gchar **argv);
};

#endif
