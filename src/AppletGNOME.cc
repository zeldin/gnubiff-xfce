/* gnubiff -- a mail notification program
 * Copyright (C) 2000-2004 Nicolas Rougier
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

#include <sstream>
#include <cstdio>
#include "AppletGNOME.h"
#include "Mailbox.h"
#include "Popup.h"
#include "gtk_image_animation.h"



// ===================================================================
// = Callbacks "C" binding ===========================================
// ===================================================================
extern "C" {
	void APPLET_GNOME_on_enter (GtkWidget *widget, GdkEventCrossing *event, gpointer data)
	{((AppletGNOME *) data)->tooltip_update ();}

	void APPLET_GNOME_on_change_orient (GtkWidget *widget, PanelAppletOrient orient, gpointer data)
	{((AppletGNOME *) data)->on_change_orient ();}

	void APPLET_GNOME_on_change_size (GtkWidget *widget, int size, gpointer data)
	{((AppletGNOME *) data)->on_change_size ();}

	void APPLET_GNOME_on_change_background (GtkWidget *widget, PanelAppletBackgroundType type, GdkColor *color, GdkPixmap *pixmap, gpointer data)
	{((AppletGNOME *) data)->on_change_background ();}

	gboolean APPLET_GNOME_on_button_press (GtkWidget *widget, GdkEventButton *event, gpointer data)
	{return ((AppletGNOME *) data)->on_button_press (event);}

	void APPLET_GNOME_on_menu_properties (BonoboUIComponent *uic, gpointer data, const gchar *verbname)
	{((AppletGNOME *) data)->on_menu_properties (uic, verbname);}

	void APPLET_GNOME_on_menu_mail_app (BonoboUIComponent *uic, gpointer data, const gchar *verbname)
	{((AppletGNOME *) data)->on_menu_mail_app (uic, verbname);}

	void APPLET_GNOME_on_menu_mail_read (BonoboUIComponent *uic, gpointer data, const gchar *verbname)
	{((AppletGNOME *) data)->on_menu_mail_read (uic, verbname);}

	void APPLET_GNOME_on_menu_about (BonoboUIComponent *uic, gpointer data, const gchar *verbname)
	{((AppletGNOME *) data)->on_menu_about (uic, verbname);}

	gboolean APPLET_GNOME_reconnect (gpointer data)
	{
		g_signal_connect (G_OBJECT (((AppletGNOME *)data)->panelapplet()),
						  "change_background",
						  GTK_SIGNAL_FUNC (APPLET_GNOME_on_change_background),
						  data);
		return FALSE;
	}
}


// ================================================================================
//  Constructors & Destructors
// --------------------------------------------------------------------------------
//  
// ================================================================================
AppletGNOME::AppletGNOME (Biff *owner, std::string xmlFilename) : Applet (owner, xmlFilename)
{
	_about = 0;
}

AppletGNOME::~AppletGNOME (void)
{
}


// ================================================================================
//  Docking
// --------------------------------------------------------------------------------
//  
// ================================================================================
void AppletGNOME::dock (GtkWidget *applet) {
	static const BonoboUIVerb gnubiffMenuVerbs [] = {
		BONOBO_UI_VERB ("Props",   APPLET_GNOME_on_menu_properties),
		BONOBO_UI_VERB ("MailApp", APPLET_GNOME_on_menu_mail_app),
		BONOBO_UI_VERB ("MailRead", APPLET_GNOME_on_menu_mail_read),
		BONOBO_UI_VERB ("About",   APPLET_GNOME_on_menu_about), 
		BONOBO_UI_VERB_END
	};
 
	panel_applet_setup_menu_from_file (PANEL_APPLET (applet),
									   NULL,
									   "GNOME_gnubiffApplet.xml",
									   NULL,
									   gnubiffMenuVerbs,
									   this);
	gtk_widget_reparent (get ("table"), applet);  
	g_signal_connect (G_OBJECT (applet), "enter_notify_event",  GTK_SIGNAL_FUNC (APPLET_GNOME_on_enter), this);
	g_signal_connect (G_OBJECT (applet), "change_orient",       GTK_SIGNAL_FUNC (APPLET_GNOME_on_change_orient), this);
	g_signal_connect (G_OBJECT (applet), "change_size",         GTK_SIGNAL_FUNC (APPLET_GNOME_on_change_size), this);
	g_signal_connect (G_OBJECT (applet), "change_background",	GTK_SIGNAL_FUNC (APPLET_GNOME_on_change_background), this);
	g_signal_connect (G_OBJECT (applet), "button_press_event",	GTK_SIGNAL_FUNC (APPLET_GNOME_on_button_press), this);


	gtk_container_set_border_width (GTK_CONTAINER (applet), 0);
	GtkTooltips *applet_tips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (applet_tips, applet, "", "");

	GtkImageAnimation *anim = new GtkImageAnimation (GTK_IMAGE(get("image")));

	g_object_set_data (G_OBJECT(get("image")), "_animation_", anim);
	anim->open (_owner->_mail_image.c_str());
	anim->start();


	gtk_widget_show (applet);
	_gnome_applet = applet;
	return;
}

// ===================================================================
// = update ==========================================================
/**
   Update applet (title, image, labels)
*/
// ===================================================================
void AppletGNOME::update (void) {
	// Is there another update going on ?
	if (!g_mutex_trylock (_update_mutex))
		return;

	std::string text;
	guint unread = unread_markup (text);
	gtk_label_set_markup (GTK_LABEL(get ("hunread")), text.c_str());
	gtk_label_set_markup (GTK_LABEL(get ("vunread")), text.c_str());


	GtkImageAnimation *anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("image")), "_animation_");
	// Pick image/animation
	if (unread > 0) {
		if (_owner->_hide_newmail_image) {
			gtk_widget_hide (get("image"));
		}
		else {
			gtk_widget_show (get("image"));
			anim->open (_owner->_mail_image.c_str());
		}
	}
	else {
		if (_owner->_hide_nomail_image) {
			gtk_widget_hide (get("image"));
		}
		else {
			gtk_widget_show (get("image"));
			anim->open (_owner->_nomail_image.c_str());
		}
	}

	guint size = panel_applet_get_size (PANEL_APPLET (_gnome_applet));
	PanelAppletOrient orient = panel_applet_get_orient (PANEL_APPLET (_gnome_applet));
	// The panel is oriented horizontally
	if ((orient == PANEL_APPLET_ORIENT_DOWN) || (orient == PANEL_APPLET_ORIENT_UP)) {
		gtk_widget_hide (get ("vunread"));
		if (((unread > 0) && (!_owner->_hide_newmail_text)) || ((unread == 0) && (!_owner->_hide_nomail_text)))
			gtk_widget_show (get ("hunread"));
		else
			gtk_widget_hide (get ("hunread"));
		if (anim->scaled_height() != size) {
			anim->stop ();
			anim->resize (anim->scaled_width()*size/anim->scaled_height(), size);
		}
	}
	// The panel is oriented vertically
	else {
		gtk_widget_hide (get ("hunread"));
		if (((unread > 0) && (!_owner->_hide_newmail_text)) || ((unread == 0) && (!_owner->_hide_nomail_text)))
			gtk_widget_show (get ("vunread"));
		else
			gtk_widget_hide (get ("vunread"));
		if (anim->scaled_width() > size) {
			anim->stop ();
			anim->resize (size, anim->scaled_height()*size/anim->scaled_width());
		}
	}


	// Background
	GtkStyle* style = gtk_style_copy (gtk_widget_get_style(_gnome_applet));
	PanelAppletBackgroundType type;
	GdkColor color;
	GdkPixmap *pixmap = NULL;
	type = panel_applet_get_background (PANEL_APPLET(_gnome_applet), &color, &pixmap);
	if (pixmap) {
		style->bg_pixmap[0] = pixmap;
		gtk_widget_set_style(_gnome_applet, style);
		gtk_widget_set_style(get("table"), style);
	}
	else {
		if (type == PANEL_NO_BACKGROUND) {
			GtkRcStyle *rc_style = gtk_rc_style_new ();
			gtk_widget_modify_style (_gnome_applet, rc_style);
			gtk_rc_style_unref (rc_style);
		}
		else {
			gtk_widget_modify_bg (get("_gnome_applet"), GTK_STATE_NORMAL, &color);
		}
	}

	if (GTK_WIDGET_VISIBLE(get("image")))
		anim->start ();
	g_mutex_unlock (_update_mutex);
}


// ================================================================================
//  Show/Hide
// --------------------------------------------------------------------------------
//  
// ================================================================================
void AppletGNOME::show (std::string name)
{
	gtk_widget_show (_gnome_applet);
}

void AppletGNOME::hide (std::string name)
{
	gtk_widget_hide (_gnome_applet);
}


// ================================================================================
//  Various callbacks
// --------------------------------------------------------------------------------
// 
// ================================================================================
void AppletGNOME::tooltip_update (void)
{
	std::string text = tooltip_text ();
	GtkTooltipsData *data = gtk_tooltips_data_get (_gnome_applet);
	gtk_tooltips_set_tip (data->tooltips, _gnome_applet, text.c_str(), "");
}

void AppletGNOME::on_change_orient (void)
{
	update();
}

void AppletGNOME::on_change_size (void) {
	update();
}

void AppletGNOME::on_change_background (void) {
	// *** HACK ***
	// From: Kurt Fitzner <kfitzner excelcia org>
    // To: desktop-devel-list gnome org
    // Subject: gnome-panel transparency problems
    // Date: Wed, 21 Apr 2004 19:44:05 -0600
	//
	// change_background signal seems to be really broken
	// so it tends to send crap quite often. One solution seems
	// to reduce the number of backgroudn update by appyling a timer
//	g_signal_handlers_disconnect_matched (_gnome_applet, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, 
//										  gpointer(APPLET_GNOME_on_change_background), this);
	update();
//	g_timeout_add (100, APPLET_GNOME_reconnect, this);
}


gboolean AppletGNOME::on_button_press (GdkEventButton *event)
{
	// Double left click : start mail app
	if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1)) {
		std::string command = _owner->_mail_app + " &";
		system (command.c_str());
	}
	// Single left click : popup menu
	else if (event->button == 1)
		watch();
	// Single middle click : mark mails as read
	else if (event->button == 2) {
		for (unsigned int i=0; i<_owner->mailboxes(); i++)
			_owner->mailbox(i)->mark_all();
		_force_popup = true;
		_owner->popup()->hide();
		_owner->applet()->process();
		_owner->applet()->update();
	}

	return FALSE;
}

void AppletGNOME::on_menu_properties (BonoboUIComponent *uic, const gchar *verbname)
{
	_owner->applet()->watch_off();
	_owner->popup()->hide();
	_owner->setup()->show();
	if (_about != 0)
		gdk_window_hide (_about->window);
}

void AppletGNOME::on_menu_mail_app (BonoboUIComponent *uic, const gchar *verbname)
{
	std::string command = _owner->_mail_app + " &";
	system (command.c_str());
}

void AppletGNOME::on_menu_mail_read (BonoboUIComponent *uic, const gchar *verbname)
{
	for (unsigned int i=0; i<_owner->mailboxes(); i++)
		_owner->mailbox(i)->mark_all();
	_force_popup = true;
	_owner->popup()->hide();
	_owner->applet()->process();
	_owner->applet()->update();
}

void AppletGNOME::on_menu_about (BonoboUIComponent *uic, const gchar *verbname)
{
	_owner->popup()->hide();
	_owner->setup()->hide();

	static const gchar *authors[] = {
		"Nicolas Rougier <Nicolas.Rougier@loria.fr>",
		NULL
	};
 
	if (_about != 0) {
		gdk_window_show (_about->window);
		gdk_window_raise (_about->window);
		return;
	}
	_about = gnome_about_new ("gnubiff applet", PACKAGE_VERSION,
							  "(C) 2000-2004\n ",
							  _("Released under the GNU general public license.\n"\
								"Mail notification program."),
							  authors,
							  NULL, NULL, gdk_pixbuf_new_from_file (GNUBIFF_DATADIR"/logo.png", 0));
	g_signal_connect (G_OBJECT (_about), "destroy", G_CALLBACK (gtk_widget_destroyed), &_about);
	gtk_widget_show (_about);
}
