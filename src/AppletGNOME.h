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

#ifndef _APPLET_GNOME_H
#define _APPLET_GNOME_H

#ifdef HAVE_CONFIG_H
#   include "../config.h"
#endif
#include <gnome.h>
#include <panel-applet.h>
#include <panel-applet-gconf.h>
#include "Applet.h"


class AppletGNOME : public Applet {

	// ===================================================================
	// - Public methods --------------------------------------------------
	// ===================================================================
 public:
	PanelApplet *panelapplet() {return(PANEL_APPLET(_gnome_applet));};

	AppletGNOME (class Biff *owner, std::string xmlFilename);
	~AppletGNOME (void);

	void dock (GtkWidget *applet);
	void update (void);
	void show (std::string name = "dialog");
	void hide (std::string name = "dialog");
	void tooltip_update (void);
	void on_change_orient (void);
	void on_change_size (void);
	void on_change_background (void);
	gboolean on_button_press (GdkEventButton *event);
	void on_menu_properties (BonoboUIComponent *uic, const gchar *verbname);
	void on_menu_mail_app (BonoboUIComponent *uic, const gchar *verbname);
	void on_menu_mail_read (BonoboUIComponent *uic, const gchar *verbname);
	void on_menu_about (BonoboUIComponent *uic, const gchar *verbname);

	// ===================================================================
	// - Private attributes ----------------------------------------------
	// ===================================================================
 private:
	GtkWidget *		_gnome_applet;		// GNOME panel applet container
	GtkWidget *		_about;				// GNOME about window
};

#endif
