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

#include "support.h"

#include <sstream>
#include <cstdio>
#include <string>
#include <glib.h>

#include "ui-preferences.h"
#include "ui-applet-gtk.h"
#include "ui-popup.h"
#include "mailbox.h"
#include "gtk_image_animation.h"



/* "C" binding */
extern "C" {
	void APPLET_GTK_on_enter (GtkWidget *widget,
							  GdkEventCrossing *event,
							  gpointer data)
	{
		if (data)
			(static_cast<AppletGtk *>(data))->tooltip_update ();
		else
			unknown_internal_error ();
	}

	gboolean APPLET_GTK_on_button_press (GtkWidget *widget,
										 GdkEventButton *event,
										 gpointer data)
	{
		if (data)
			return (static_cast<AppletGtk *>(data))->on_button_press (event);
		else
			unknown_internal_error ();
		return false;
	}

	void APPLET_GTK_on_menu_command (GtkWidget *widget,
									 gpointer data)
	{
		if (data)
			(static_cast<AppletGtk *>(data))->execute_command ("double_command",
												   "use_double_command");
		else
			unknown_internal_error ();
	}

	void APPLET_GTK_on_menu_mark (GtkWidget *widget,
								  gpointer data)
	{
		if (data)
			(static_cast<AppletGtk *>(data))->mark_messages_as_read ();
		else
			unknown_internal_error ();
	}

	void APPLET_GTK_on_menu_preferences (GtkWidget *widget, gpointer data)
	{
		if (data)
			(static_cast<AppletGtk *>(data))->show_dialog_preferences ();
		else
			unknown_internal_error ();
	}

	void APPLET_GTK_on_menu_about (GtkWidget *widget, gpointer data)
	{
		if (data)
			(static_cast<AppletGtk *>(data))->show_dialog_about ();
		else
			unknown_internal_error ();
	}
	
	void APPLET_GTK_on_menu_quit (GtkWidget *widget,
								  gpointer data)
	{
		if (data)
			(static_cast<AppletGtk *>(data))->on_menu_quit ();
		else
			unknown_internal_error ();
	}
}

// ============================================================================
//  base
// ============================================================================
/**
 *  Constructor.
 *
 *  @param  biff  Pointer to gnubiff's biff
 */
AppletGtk::AppletGtk (Biff *biff)
		  : AppletGUI (biff, GNUBIFF_DATADIR"/applet-gtk.ui", this)
{
	tooltip_widget_ = GTK_WIDGET (get ("dialog"));
}

/**
 *  Constructor to be called from a constructor in a subclass of AppletGtk.
 *
 *  @param  biff   Pointer to gnubiff's biff
 *  @param  applet Pointer to the applet itself
 */
AppletGtk::AppletGtk (class Biff *biff, class Applet *applet)
		  : AppletGUI (biff, GNUBIFF_DATADIR"/applet-gtk.ui", applet)
{
}

/// Destructor
AppletGtk::~AppletGtk (void)
{
}

// ============================================================================
//  main
// ============================================================================

/**
 *  Update applet's status and appearance.
 *
 *  @param  init  True if called when initializing gnubiff (the default is
 *                false)
 *  @return       True if new messages are present
 */
gboolean 
AppletGtk::update (gboolean init)
{
	// Is there another update going on?
	if (!g_mutex_trylock (update_mutex_))
		return false;

	gboolean newmail = AppletGUI::update (init, "image", "unread", "fixed");

	tooltip_update ();
	show ();

	g_mutex_unlock (update_mutex_);
	return newmail;
}

/**
 *  Show the applet. Also the applet's appearance is updated.
 *
 *  @param  name  This parameter is ignored (the default is "dialog").
 */
void 
AppletGtk::show (std::string name)
{
	GtkWindow *dialog = GTK_WINDOW (get ("dialog"));

	// Update window manager decorations
	gboolean decorated = gtk_window_get_decorated (dialog);
	if (decorated != biff_->value_bool ("applet_use_decoration"))
		gtk_window_set_decorated (dialog,
								  biff_->value_bool ("applet_use_decoration"));

	gtk_widget_show (GTK_WIDGET (dialog));

	if (biff_->value_bool ("applet_use_geometry"))
		gtk_window_parse_geometry (dialog,
								   biff_->value_string ("applet_geometry").c_str());
	if (biff_->value_bool ("applet_be_sticky"))
		gtk_window_stick (dialog);
	else
		gtk_window_unstick (dialog);
	gtk_window_set_keep_above(dialog, biff_->value_bool ("applet_keep_above"));
	gtk_window_set_skip_pager_hint (dialog,!biff_->value_bool("applet_pager"));
	gtk_window_set_skip_taskbar_hint (dialog,
									  !biff_->value_bool ("applet_taskbar"));
}

gboolean 
AppletGtk::on_button_press (GdkEventButton *event)
{
	// Double left click: start mail app
	if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1))
		execute_command ("double_command", "use_double_command");

	// Single left click: force mail check
	else if (event->button == 1) {
		force_popup_ = true;
		update ();
	}

	// Single middle click: mark messages as read
	else if (event->button == 2)
		mark_messages_as_read ();

	// Single right click: popup menu
	else if (event->button == 3) {
		if (biff_->value_bool ("use_double_command"))
			gtk_widget_set_sensitive (get("menu_start_command"), true);
		else
			gtk_widget_set_sensitive (get("menu_start_command"), false);
		gtk_menu_popup (GTK_MENU(get("menu")), NULL, NULL, NULL, NULL,
						event->button, event->time);
	}
	return true;
}

void
AppletGtk::on_menu_quit (void)
{
	gtk_main_quit();
}
