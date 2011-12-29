// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2011 Nicolas Rougier, 2004-2011 Robert Sowada
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

#include "ui-applet-gnome.h"
#include "ui-popup.h"
#include "mailbox.h"
#include "gtk_image_animation.h"



/**
 * "C" binding
 **/
extern "C" {
	void APPLET_GNOME_on_enter (GtkWidget *widget,
								GdkEventCrossing *event,
								gpointer data)
	{
		if (data)
			(static_cast<AppletGnome *>(data))->tooltip_update ();
		else
			unknown_internal_error ();
	}

	void APPLET_GNOME_on_size_allocate (GtkWidget *widget,
										GtkAllocation *allocation,
										gpointer data)
	{
		if (!data)
			unknown_internal_error ();
		else
		  	if (!(static_cast<AppletGnome *>(data))->calculate_size (allocation))
				(static_cast<AppletGnome *>(data))->update ();
	}

// 	void APPLET_GNOME_on_change_background (GtkWidget *widget,
// 											PanelAppletBackgroundType type,
// 											GdkColor *color,
// 											GdkPixmap *pixmap,
// 											gpointer data)
// 	{
//  		if (data)
//  			(static_cast<AppletGnome *>(data))->update ();
//  		else
//  			unknown_internal_error ();
// 	}

	gboolean APPLET_GNOME_on_button_press (GtkWidget *widget,
										   GdkEventButton *event,
										   gpointer data)
	{
		if (data)
			return (static_cast<AppletGnome *>(data))->on_button_press (event);
		else
			unknown_internal_error ();
		return false;
	}

	static void APPLET_GNOME_on_menu_properties (GtkAction *action,
                                                 AppletGnome *data)
	{
		if (data)
			data->show_dialog_preferences ();
		else
			unknown_internal_error ();
	}

	void APPLET_GNOME_on_menu_command (GtkAction *action,
                                       AppletGnome *data)
	{
		if (data)
			data->execute_command ("double_command", "use_double_command");
		else
			unknown_internal_error ();
	}

	void APPLET_GNOME_on_menu_mail_read (GtkAction *action,
                                         AppletGnome *data)
	{
		if (data)
			data->mark_messages_as_read ();
		else
			unknown_internal_error ();
	}

	void APPLET_GNOME_on_menu_info (GtkAction *action,
                                    AppletGnome *data)
	{
		if (data)
			data->show_dialog_about ();
		else
			unknown_internal_error ();
	}

// 	gboolean APPLET_GNOME_reconnect (gpointer data)
// 	{
// 		if (data) {
// 			g_signal_connect (G_OBJECT ((static_cast<AppletGnome *>(data))->panelapplet()),
// 							  "change_background",
// 							  GTK_SIGNAL_FUNC (APPLET_GNOME_on_change_background),
// 							  data);
// 		}
// 		else
// 			unknown_internal_error ();
// 		return false;
// 	}
}

// ========================================================================
//  base
// ========================================================================

AppletGnome::AppletGnome (Biff *biff) : AppletGUI (biff, GNUBIFF_DATADIR"/applet-gtk.ui", this)
{
}

AppletGnome::~AppletGnome (void)
{
}

// ========================================================================
//  tools
// ========================================================================
/**
 *  Return the panel's orientation.
 *
 *  @param  orient Panel's orientation
 *  @return        Boolean indicating success
 */
gboolean 
AppletGnome::get_orientation (GtkOrientation &orient)
{
	switch (panel_applet_get_orient (PANEL_APPLET(applet_))) {
	case PANEL_APPLET_ORIENT_DOWN:
	case PANEL_APPLET_ORIENT_UP:
		orient = GTK_ORIENTATION_HORIZONTAL;
		break;
	case PANEL_APPLET_ORIENT_LEFT:
	case PANEL_APPLET_ORIENT_RIGHT:
		orient = GTK_ORIENTATION_VERTICAL;
		break;
	default:
		// Should never happen
		return false;
	}
	return true;
}

// ========================================================================
//  main
// ========================================================================
/**
 *  Set properties of the gnubiff gnome panel applet.
 *
 *  @param applet  Gnome Panel widget of gnubiff.
 */
void 
AppletGnome::dock (GtkWidget *applet)
{
	// Create the applet's menu
    static const GtkActionEntry gnubiff_menu_actions [] = {
        { "Props",    GTK_STOCK_PROPERTIES, N_("_Preferences..."), NULL, NULL, G_CALLBACK (APPLET_GNOME_on_menu_properties) },
        { "MailApp",  GTK_STOCK_PROPERTIES, N_("_Run command"), NULL, NULL, G_CALLBACK (APPLET_GNOME_on_menu_command) },
        { "MailRead", GTK_STOCK_PROPERTIES, N_("_Mark mailboxes read"), NULL, NULL, G_CALLBACK (APPLET_GNOME_on_menu_mail_read) },
        { "Info",     GTK_STOCK_ABOUT,      N_("_Info"), NULL, NULL, G_CALLBACK (APPLET_GNOME_on_menu_info) }
    };
    GtkActionGroup * action_group = gtk_action_group_new ("ShowDesktop Applet Actions");
    gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
    gtk_action_group_add_actions (action_group, gnubiff_menu_actions, G_N_ELEMENTS (gnubiff_menu_actions), this);
    gchar *ui_path = g_build_filename (GNUBIFF_UIDIR, "GNOME_gnubiffApplet.xml", NULL);
    panel_applet_setup_menu_from_file (PANEL_APPLET (applet), ui_path, action_group);
    g_free (ui_path);
    g_object_unref (action_group);

	// We need the PANEL_APPLET_EXPAND_MINOR for getting the correct size of
	// the gnome panel via the "size_allocate" signal
	panel_applet_set_flags (PANEL_APPLET (applet), PANEL_APPLET_EXPAND_MINOR);

	// Put gnubiff's widgets inside the panel's applet
	gtk_widget_reparent (get ("fixed"), applet);  
	gtk_container_set_border_width (GTK_CONTAINER (applet), 0);

	// Tooltips
    gtk_widget_set_tooltip_text (applet, "");

	// Connect callback functions to the applet's signals
	g_signal_connect (G_OBJECT (applet), "enter_notify_event",
					  G_CALLBACK (APPLET_GNOME_on_enter), this);
	g_signal_connect (G_OBJECT (applet), "size_allocate",
					  G_CALLBACK (APPLET_GNOME_on_size_allocate), this);
// 	g_signal_connect (G_OBJECT (applet), "change_background",
// 					  G_CALLBACK (APPLET_GNOME_on_change_background), this);
	g_signal_connect (G_OBJECT (applet), "button_press_event",
					  G_CALLBACK (APPLET_GNOME_on_button_press), this);

	applet_ = applet;
}

gboolean 
AppletGnome::update (gboolean init)
{
	// Is there another update going on?
	if (!g_mutex_trylock (update_mutex_))
		return false;

	// Update applet (depending on the orientation of the panel)
	gboolean newmail = AppletGUI::update (init, "image", "unread", "fixed");

	// Background
// 	PanelAppletBackgroundType type;
// 	GdkColor color;
// 	GdkPixmap *pixmap = NULL;
// 	type = panel_applet_get_background (PANEL_APPLET (applet_), &color,
// 										&pixmap);
// 	if (pixmap && G_IS_OBJECT(pixmap)) {		
// 		GtkStyle* style = gtk_style_copy (gtk_widget_get_style (applet_));
// 		style->bg_pixmap[0] = pixmap;
// 		gtk_widget_set_style (applet_, style);
// 		gtk_widget_set_style (get("fixed"), style);
// 		g_object_unref (style);
// 	}
// 	else {
// 		if (type == PANEL_NO_BACKGROUND) {
// 			GtkRcStyle *rc_style = gtk_rc_style_new ();
// 			gtk_widget_modify_style (applet_, rc_style);
// 			gtk_rc_style_unref (rc_style);
// 		}
// 		else
// 			gtk_widget_modify_bg (get("applet_"), GTK_STATE_NORMAL, &color);
// 	}

	g_mutex_unlock (update_mutex_);

	return newmail;
}


void
AppletGnome::show (std::string name)
{
	gtk_widget_show (applet_);
}

void
AppletGnome::hide (std::string name)
{
	gtk_widget_hide (applet_);
}

/**
 *  Calculate the applet's size. This function should be called when a
 *  "size_allocate" signal is caught.
 *
 *  @param  allocation  Parameter passed to the "size_allocate" callback.
 *  @return             True, if there is no change in the applet's size,
 *                      false otherwise.
 */
gboolean 
AppletGnome::calculate_size (GtkAllocation *allocation)
{
	guint widget_max_height_old = widget_max_height_;
	guint widget_max_width_old = widget_max_width_;

	// Check parameters
	if (!allocation)
		return true;

	// Get the orientation of the panel
	PanelAppletOrient orient = panel_applet_get_orient (PANEL_APPLET(applet_));

	// Determine the new maximum size of the applet (depending on the
	// orientation)
	widget_max_height_ = G_MAXUINT;
	widget_max_width_ = G_MAXUINT;
	switch (orient) {
	case PANEL_APPLET_ORIENT_DOWN:
	case PANEL_APPLET_ORIENT_UP:
		widget_max_height_ = allocation->height;
		break;
	case PANEL_APPLET_ORIENT_LEFT:
	case PANEL_APPLET_ORIENT_RIGHT:
		widget_max_width_ = allocation->width;
		break;
	default:
		// Should never happen
		break;
	}

	return ((widget_max_height_old == widget_max_height_) &&
			(widget_max_width_old == widget_max_width_));;
}

// ============================================================================
//  callbacks
// ============================================================================

gboolean
AppletGnome::on_button_press (GdkEventButton *event)
{
	// Double left click: start mail app
	if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1))
		execute_command ("double_command", "use_double_command");
	// Single left click: popup menu
	else if (event->button == 1) {
		force_popup_ = true;
		update ();
	}
	// Single middle click: mark messages as read
	else if (event->button == 2)
		mark_messages_as_read ();	

	return false;
}

/**
 *  This callback is called when gnome panel applet has been created. 
 *
 *  @param  applet Pointer to the panel applet.
 *  @param  iid    FIXME
 *  @param  data   This is currently always the NULL pointer.
 *  @return        Always true.
 */
gboolean 
AppletGnome::gnubiff_applet_factory (PanelApplet *applet, const gchar *iid,
									 gpointer data)
{
	if (strcmp (iid, "GnubiffApplet"))
	  return true;

	Biff *biff = new Biff (MODE_GNOME);
	AppletGnome *biffapplet = static_cast<AppletGnome *>( biff->applet() );
	biffapplet->dock ((GtkWidget *) applet);
	biffapplet->start (false);
	return true;
}
