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

#ifndef __APPLET_GNOME_H__
#define __APPLET_GNOME_H__

#include <gnome.h>
#include <panel-applet.h>
#include <panel-applet-gconf.h>
#include "ui-applet.h"


class AppletGnome : public Applet {

protected:
	GtkWidget *		applet_;
	GtkWidget *		about_;

public:
	/* base */
	AppletGnome (class Biff *biff);
	~AppletGnome (void);

	/* main */
	PanelApplet *panelapplet() {return(PANEL_APPLET(applet_));};
	void dock (GtkWidget *applet);
	void update (void);
	void show (std::string name = "dialog");
	void hide (std::string name = "dialog");
	void tooltip_update (void);

	/* callbacks */
	gboolean on_button_press (GdkEventButton *event);
	void on_menu_properties (BonoboUIComponent *uic, const gchar *verbname);
	void on_menu_mail_app (BonoboUIComponent *uic, const gchar *verbname);
	void on_menu_mail_read (BonoboUIComponent *uic, const gchar *verbname);
	void on_menu_about (BonoboUIComponent *uic, const gchar *verbname);
};

#endif
