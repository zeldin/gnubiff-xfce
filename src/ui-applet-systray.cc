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
// Author(s)     : Nicolas Rougier
// Short         : 
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#include <glib.h>
#include <gtk/gtk.h>

#include "eggtrayicon.h"
#include "ui-applet-systray.h"
#include "support.h"


// ============================================================================
//  base
// ============================================================================

/**
 *  Constructor.
 *
 *  @param  biff  Pointer to gnubiff's biff
 */
AppletSystray::AppletSystray (Biff *biff) : AppletGtk (biff, this)
{
	// Create the system tray icon
	trayicon_ = egg_tray_icon_new ("trayicon");

	// Connect signals to system tray icon
	g_signal_connect (G_OBJECT (trayicon_), "size-allocate",
					  GTK_SIGNAL_FUNC (signal_size_allocate), this);

	// Tooltips shall be displayed in the system tray
	GtkTooltips *applet_tips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (applet_tips, GTK_WIDGET (trayicon_), "", "");
	tooltip_widget_ = GTK_WIDGET (trayicon_);

	// We want to reuse the widgets for AppletGtk. So we have to change the
	// parent from the top level window to the system tray icon
	GtkWidget *eventbox = GTK_WIDGET (get ("event"));
	gtk_widget_reparent (eventbox, GTK_WIDGET (trayicon_));

	// Show the tray icon
	gtk_widget_show_all (GTK_WIDGET (trayicon_));
}

/// Destructor.
AppletSystray::~AppletSystray (void)
{
}

// ============================================================================
//  main
// ============================================================================

/**
 *  Show the applet. Also the applet's appearance is updated.
 *
 *  @param  name  This parameter is ignored (the default is "dialog").
 */
void 
AppletSystray::show (std::string name)
{
	gtk_widget_show (GTK_WIDGET (trayicon_));
}

/**
 *  This function is called automatically when the system tray icon is resized.
 *
 *  @param  width  New width of the icon.
 *  @param  height New height of the icon.
 */
void 
AppletSystray::resize (guint width, guint height)
{
	// Get image's current size
	guint ic_width = 0, ic_height = 0;
	get_image_size ("image", ic_width, ic_height);

	// Do we need to have the image rescaled?
	if (((ic_width != width) || (ic_height > height))
		&& ((ic_width > width) || (ic_height != height))) {
		widget_max_width_ = width;
		widget_max_height_ = height;
		update (); 
	}
}

// ============================================================================
//  callbacks
// ============================================================================
/**
 *  Callback function that is called when the size of the system tray icon
 *  is changed. This function calls
 *
 *  @param  widget     System tray icon
 *  @param  allocation Position and size of {\em widget}
 *  @param  user_data  Pointer to the corresponding AppletSystray object
 */
void 
AppletSystray::signal_size_allocate (GtkWidget *widget,
									 GtkAllocation *allocation, gpointer data)
{
	if (data)
		((AppletSystray *) data)->resize (allocation->width,
										  allocation->height);
	else
		unknown_internal_error ();
}
