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
			((AppletGtk *) data)->tooltip_update ();
		else
			unknown_internal_error ();
	}

	gboolean APPLET_GTK_on_button_press (GtkWidget *widget,
										 GdkEventButton *event,
										 gpointer data)
	{
		if (data)
			return ((AppletGtk *) data)->on_button_press (event);
		else
			unknown_internal_error ();
		return false;
	}

	void APPLET_GTK_on_menu_command (GtkWidget *widget,
									 gpointer data)
	{
		if (data)
			((AppletGtk *) data)->execute_command ("double_command",
												   "use_double_command");
		else
			unknown_internal_error ();
	}

	void APPLET_GTK_on_menu_mark (GtkWidget *widget,
								  gpointer data)
	{
		if (data)
			((AppletGtk *) data)->mark_messages_as_read ();
		else
			unknown_internal_error ();
	}

	void APPLET_GTK_on_menu_preferences (GtkWidget *widget, gpointer data)
	{
		if (data)
			((AppletGtk *) data)->show_dialog_preferences ();
		else
			unknown_internal_error ();
	}

	void APPLET_GTK_on_menu_about (GtkWidget *widget, gpointer data)
	{
		if (data)
			((AppletGtk *) data)->show_dialog_about ();
		else
			unknown_internal_error ();
	}
	
	void APPLET_GTK_on_menu_quit (GtkWidget *widget,
								  gpointer data)
	{
		if (data)
			((AppletGtk *) data)->on_menu_quit ();
		else
			unknown_internal_error ();
	}

	void APPLET_GTK_on_hide_about (GtkWidget *widget, gpointer data)
	{
		if (data)
			((AppletGtk *) data)->hide_dialog_about ();
		else
			unknown_internal_error ();
	}
}

AppletGtk::AppletGtk (Biff *biff) : AppletGUI (biff, GNUBIFF_DATADIR"/applet-gtk.glade", this)
{
}

AppletGtk::~AppletGtk (void)
{
}

gint
AppletGtk::create (gpointer callbackdata)
{
	GUI::create(this);
	GtkImageAnimation *anim = new GtkImageAnimation (GTK_IMAGE(get("image")));
	g_object_set_data (G_OBJECT(get("image")), "_animation_", anim);
	anim->open (biff_->value_string ("nomail_image"));
	anim->start();
	update();
	return true;
}

void
AppletGtk::update (gboolean no_popup)
{
	// Is there another update going on ?
	if (!g_mutex_trylock (update_mutex_))
		return;

	AppletGUI::update (no_popup, "image", "unread", "fixed");

	// Update window manager decorations
	gboolean decorated = gtk_window_get_decorated (GTK_WINDOW(get("dialog")));
	if (decorated != biff_->value_bool ("applet_use_decoration"))
		gtk_window_set_decorated (GTK_WINDOW(get("dialog")),
								  biff_->value_bool ("applet_use_decoration"));

	tooltip_update();
	show();

	g_mutex_unlock (update_mutex_);
}

void
AppletGtk::show (std::string name)
{
	int unread = 0;
	for (unsigned int i=0; i<biff_->size(); i++)
		unread += biff_->mailbox(i)->unreads();

	if ((unread > 0) && (!biff_->value_bool ("use_newmail_image")))
		return;
	else if (!biff_->value_bool ("use_nomail_image"))
		return;

	GtkWindow *dialog=GTK_WINDOW(get("dialog"));
	gtk_widget_show (get("dialog"));
	if (biff_->value_bool ("applet_use_geometry"))
		gtk_window_parse_geometry (dialog, biff_->value_gchar ("applet_geometry"));
	if (biff_->value_bool ("applet_be_sticky"))
		gtk_window_stick (dialog);
	else
		gtk_window_unstick (dialog);
	gtk_window_set_keep_above(dialog, biff_->value_bool ("applet_keep_above"));
	gtk_window_set_skip_pager_hint (dialog,!biff_->value_bool("applet_pager"));
}

void
AppletGtk::tooltip_update (void)
{
	std::string tooltip = get_mailbox_status_text ();
	GtkTooltipsData *tt = gtk_tooltips_data_get  (get("dialog"));
	gtk_tooltips_set_tip (tt->tooltips, get("dialog"), tooltip.c_str(), "");
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
