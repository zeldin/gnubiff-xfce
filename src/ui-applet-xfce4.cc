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

#include "ui-applet-xfce4.h"
#include "ui-popup.h"
#include "mailbox.h"
#include "gtk_image_animation.h"


/**
 * "C" binding
 **/
extern "C" {
    void APPLET_XFCE4_on_enter (GtkWidget *widget,
                                GdkEventCrossing *event,
                                gpointer data)
    {
        if (data)
            (static_cast<AppletXfce4 *>(data))->tooltip_update ();
        else
            unknown_internal_error ();
    }

    gboolean APPLET_XFCE4_on_size_changed (XfcePanelPlugin *plugin,
                                           gint size,
                                           gpointer data)
    {
        if (!data) {
            unknown_internal_error ();
            return FALSE;
        } else
            return (static_cast<AppletXfce4 *>(data))->on_size_changed (plugin, size);
    }

    void APPLET_XFCE4_on_orientation_changed (XfcePanelPlugin *plugin,
                                              GtkOrientation,
                                              gpointer data)
    {
        if (!data)
            unknown_internal_error ();
        else
            (static_cast<AppletXfce4 *>(data))->on_size_changed (plugin,
                                                                 xfce_panel_plugin_get_size (plugin));
    }

    gboolean APPLET_XFCE4_on_button_press (GtkWidget *widget,
                                           GdkEventButton *event,
                                           gpointer data)
    {
        if (data)
            return (static_cast<AppletXfce4 *>(data))->on_button_press (event);
        else
            unknown_internal_error ();
        return false;
    }

    void APPLET_XFCE4_on_configure_plugin (XfcePanelPlugin *plugin,
                                           gpointer data)
    {
        if (data)
            return (static_cast<AppletXfce4 *>(data))->show_dialog_preferences ();
        else
            unknown_internal_error ();
    }

    void APPLET_XFCE4_on_about (XfcePanelPlugin *plugin,
                                gpointer data)
    {
        if (data)
            return (static_cast<AppletXfce4 *>(data))->show_dialog_about ();
        else
            unknown_internal_error ();
    }

    void APPLET_XFCE4_gnubiff_applet_construct (XfcePanelPlugin *plugin)
    {
        AppletXfce4::gnubiff_applet_construct (plugin);
    }

    gboolean APPLET_XFCE4_gnubiff_applet_preinit (gint argc, gchar **argv)
    {
        return AppletXfce4::gnubiff_applet_preinit (argc, argv);
    }
}

// ========================================================================
//  base
// ========================================================================

AppletXfce4::AppletXfce4 (Biff *biff) : AppletGUI (biff, GNUBIFF_DATADIR"/applet-gtk.ui", this)
{
}

AppletXfce4::~AppletXfce4 (void)
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
AppletXfce4::get_orientation (GtkOrientation &orient)
{
    orient = xfce_panel_plugin_get_orientation (plugin_);
    return true;
}

// ========================================================================
//  main
// ========================================================================
/**
 *  Set properties of the gnubiff xfce4 panel plugin.
 *
 *  @param plugin  Xfce4 Panel plugin of gnubiff.
 */
void 
AppletXfce4::dock (XfcePanelPlugin *plugin)
{
    // Create the applet's menu
    GtkUIManager *uiman = GTK_UI_MANAGER (gtk_builder_get_object (gtkbuilder_, "uimanager1"));
    GtkWidget *item;
    xfce_panel_plugin_menu_show_configure (plugin);
    item = gtk_ui_manager_get_widget (uiman, "/menu/menu_start_command");
    gtk_widget_unparent (item);
    xfce_panel_plugin_menu_insert_item (plugin, GTK_MENU_ITEM(item));
    item = gtk_ui_manager_get_widget (uiman, "/menu/menu_mark_mails");
    gtk_widget_unparent (item);
    xfce_panel_plugin_menu_insert_item (plugin, GTK_MENU_ITEM(item));
    xfce_panel_plugin_menu_show_about (plugin);

    // We need the xfce_panel_plugin_set_expand for getting the correct
    // size of the xfce4 panel via the "size_allocate" signal
    xfce_panel_plugin_set_expand (plugin, FALSE);

    // Put gnubiff's widgets inside the panel's applet
    alignment_ = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
    gtk_widget_reparent (get ("fixed"), alignment_);
    gtk_widget_show (alignment_);
    gtk_container_add(GTK_CONTAINER(plugin), alignment_);
    gtk_container_set_border_width (GTK_CONTAINER (plugin), 0);

    // Tooltips
    gtk_widget_set_tooltip_text (GTK_WIDGET(plugin), "");
    tooltip_widget_ = GTK_WIDGET(plugin);

    // Connect callback functions to the applet's signals
    g_signal_connect (G_OBJECT (plugin), "size-changed",
                      G_CALLBACK (APPLET_XFCE4_on_size_changed), this);
    g_signal_connect (G_OBJECT (plugin), "orientation-changed",
                      G_CALLBACK (APPLET_XFCE4_on_orientation_changed), this);
    g_signal_connect (G_OBJECT (plugin), "enter_notify_event",
		      G_CALLBACK (APPLET_XFCE4_on_enter), this);
    g_signal_connect (G_OBJECT (plugin), "button_press_event",
		      G_CALLBACK (APPLET_XFCE4_on_button_press), this);
    g_signal_connect (G_OBJECT (plugin), "configure-plugin",
		      G_CALLBACK (APPLET_XFCE4_on_configure_plugin), this);
    g_signal_connect (G_OBJECT (plugin), "about",
		      G_CALLBACK (APPLET_XFCE4_on_about), this);

    plugin_ = plugin;
}

gboolean 
AppletXfce4::update (gboolean init)
{
    // Is there another update going on?
    if (!g_mutex_trylock (update_mutex_))
        return false;

    // Update applet (depending on the orientation of the panel)
    gboolean newmail = AppletGUI::update (init, "image", "unread", "fixed");

    g_mutex_unlock (update_mutex_);

    return newmail;
}

void
AppletXfce4::show (std::string name)
{
	gtk_widget_show (GTK_WIDGET (plugin_));
}

void
AppletXfce4::hide (std::string name)
{
	gtk_widget_hide (GTK_WIDGET (plugin_));
}

/**
 *  Calculate the applet's size. This function should be called when a
 *  "size-changed" signal is caught.
 *
 *  @param  size        Parameter passed to the "size-changed" callback.
 *  @return             True, if there is no change in the applet's size,
 *                      false otherwise.
 */
gboolean 
AppletXfce4::calculate_size (gint size)
{
	guint widget_max_height_old = widget_max_height_;
	guint widget_max_width_old = widget_max_width_;

	// Get the orientation of the panel
	GtkOrientation orient = xfce_panel_plugin_get_orientation (plugin_);

	// Determine the new maximum size of the applet (depending on the
	// orientation)
	widget_max_height_ = G_MAXUINT;
	widget_max_width_ = G_MAXUINT;
	switch (orient) {
	case GTK_ORIENTATION_HORIZONTAL:
		widget_max_height_ = size;
		break;
	case GTK_ORIENTATION_VERTICAL:
		widget_max_width_ = size;
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
AppletXfce4::on_button_press (GdkEventButton *event)
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

gboolean
AppletXfce4::on_size_changed (XfcePanelPlugin *plugin, gint size)
{
    if (!calculate_size (size))
        update ();

    return TRUE;
}


/**
 *  This callback is called when xfce4 panel applet has been created. 
 *
 *  @param  plugin Pointer to the panel plugin.
 */
void
AppletXfce4::gnubiff_applet_construct (XfcePanelPlugin *plugin)
{
    Biff *biff = new Biff (MODE_XFCE4);
    AppletXfce4 *biffapplet = static_cast<AppletXfce4 *>( biff->applet() );
    biffapplet->dock (plugin);
    biffapplet->start (false);
}

gboolean
AppletXfce4::gnubiff_applet_preinit (gint argc, gchar **argv)
{
    // Initialize i18n support
    setlocale (LC_ALL, "");
#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, GNUBIFF_LOCALEDIR);
#   ifdef HAVE_BIND_TEXTDOMAIN_CODESET
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#   endif
    textdomain (GETTEXT_PACKAGE);
#endif

    // Thread initialization
    g_thread_init (NULL);
    gdk_threads_init ();

    g_set_application_name("gnubiff");

    return TRUE;
}
