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

#include <sstream>
#include <cstdio>
#include <string>
#include <glib.h>
#include "AppletGTK.h"
#include "Mailbox.h"
#include "Popup.h"
#include "gtk_image_animation.h"



// ================================================================================
//  Callbacks (C binding)
// ================================================================================
extern "C" {
	void APPLET_GTK_on_enter (GtkWidget *widget, GdkEventCrossing *event, gpointer data)
	{((AppletGTK *) data)->tooltip_update ();}
	gboolean APPLET_GTK_on_button_press (GtkWidget *widget, GdkEventButton *event, gpointer data)
	{return ((AppletGTK *) data)->on_button_press (event);}
	void APPLET_GTK_on_menu_mail_app (GtkWidget *widget, gpointer data)
	{((AppletGTK *) data)->on_menu_mail_app ();}
	void APPLET_GTK_on_menu_mark (GtkWidget *widget, gpointer data)
	{((AppletGTK *) data)->on_menu_mark ();}
	void APPLET_GTK_on_menu_preferences (GtkWidget *widget, gpointer data)
	{((AppletGTK *) data)->on_menu_preferences ();}
	void APPLET_GTK_on_menu_about (GtkWidget *widget, gpointer data)
	{((AppletGTK *) data)->on_menu_about ();}
	void APPLET_GTK_on_hide_about (GtkWidget *widget, gpointer data)
	{((AppletGTK *) data)->on_hide_about ();}
}



// ================================================================================
//  Constructors & Destructors
// --------------------------------------------------------------------------------
//  
// ================================================================================
AppletGTK::AppletGTK (Biff *owner, std::string xmlFilename) : Applet (owner, xmlFilename)
{
}

AppletGTK::~AppletGTK (void)
{
}

gint AppletGTK::create (void) {
	GUI::create();
	GtkImageAnimation *anim = new GtkImageAnimation (GTK_IMAGE(get("image")));
	g_object_set_data (G_OBJECT(get("image")), "_animation_", anim);
	anim->open (_owner->_nomail_image.c_str());
	anim->start();
	return true;
}


// ================================================================================
//  Update applet according to mailboxes state
// --------------------------------------------------------------------------------
//  
// ================================================================================
void AppletGTK::update (void) {
	// Is there another update going on ?
	if (!g_mutex_trylock (_update_mutex))
		return;

	std::string text;
	guint unread = unread_markup (text);

	// Image/animation
	GtkImageAnimation *anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("image")), "_animation_");
	if (unread > 0) {
		anim->open (_owner->_mail_image.c_str());
		anim->start();
		if (_owner->_hide_newmail_image)
			hide();
		else
			show ();
		if (_owner->_hide_newmail_text)
			gtk_widget_hide (get("unread"));
		else
			gtk_widget_show (get("unread"));
	}
	else {
		anim->open (_owner->_nomail_image.c_str());
		anim->start();
		if (_owner->_hide_nomail_image)
			hide();
		else
			show ();
		if (_owner->_hide_nomail_text)
			gtk_widget_hide (get("unread"));
		else
			gtk_widget_show (get("unread"));
	}

	gtk_label_set_markup (GTK_LABEL(get ("unread")), text.c_str());
	
	// Update widgets relative to image size
	guint width = anim->scaled_width();
	guint height = anim->scaled_height();
	gtk_widget_set_size_request (get("fixed"), width, height);
	gtk_widget_set_size_request (get("image"), width, height);
	gtk_widget_set_size_request (get("unread"), width, -1);

	GtkRequisition req;
	gtk_widget_size_request (get("unread"), &req);
	gtk_fixed_move (GTK_FIXED(get("fixed")), get("unread"), 0, height-req.height);

	// Update window manager decorations
	gtk_window_set_decorated (GTK_WINDOW(get("dialog")), _owner->_biff_decorated);
	tooltip_update();
	show();

	g_mutex_unlock (_update_mutex);
}


// ================================================================================
//  Show the applet
// --------------------------------------------------------------------------------
//  Show the applet (at the right place if necessary)
// ================================================================================
void AppletGTK::show (std::string name)
{
	// Do we really want to show the applet ?
	int unread = 0;
	for (unsigned int i=0; i<_owner->mailboxes(); i++)
		unread += _owner->mailbox(i)->unreads();
	if ((unread > 0) && (_owner->_hide_newmail_image))
		return;
	else if (_owner->_hide_nomail_image)
		return;

	std::string geometry  = "";
	if (_owner->_biff_x_sign == SIGN_PLUS)
		geometry += "+";
	else
		geometry += "-";
	std::stringstream x;
	x << _owner->_biff_x;
	geometry += x.str();

	if (_owner->_biff_y_sign == SIGN_PLUS)
		geometry += "+";
	else
		geometry += "-";
	std::stringstream y;
	y << _owner->_biff_y;
	geometry += y.str();

	gtk_widget_show (get("dialog"));

	if (!_owner->_biff_no_geometry)
		gtk_window_parse_geometry (GTK_WINDOW(get("dialog")), geometry.c_str());
}


// ================================================================================
//  Various callbacks
// --------------------------------------------------------------------------------
// 
// ================================================================================
void AppletGTK::tooltip_update (void)
{
	std::string tooltip = tooltip_text ();
	GtkTooltipsData *tt = gtk_tooltips_data_get  (get("dialog"));
	gtk_tooltips_set_tip (tt->tooltips, get("dialog"), tooltip.c_str(), "");
}

gboolean AppletGTK::on_button_press (GdkEventButton *event)
{

	// Double left click : start mail app
	if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1))
		on_menu_mail_app ();

	// Single left click : force mail check
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

	// Single right click : popup menu
	else if (event->button == 3)
		gtk_menu_popup (GTK_MENU(get("menu")), NULL, NULL, NULL, NULL, event->button, event->time);
	return true;
}

// ================================================================================
void AppletGTK::on_menu_mail_app (void)
{
	std::string command = _owner->_mail_app + " &";
	system (command.c_str());	
}

// ================================================================================
void AppletGTK::on_menu_preferences (void)
{
	_owner->applet()->watch_off();
	_owner->popup()->hide();
	_owner->setup()->show();
}

// ================================================================================
void AppletGTK::on_menu_mark (void)
{
	for (unsigned int i=0; i<_owner->mailboxes(); i++)
		_owner->mailbox(i)->mark_all();
	_force_popup = true;
	_owner->popup()->hide();
	_owner->applet()->process();
	_owner->applet()->update();
}

// ================================================================================
void AppletGTK::on_menu_about (void)
{
	_owner->popup()->hide();
	_owner->setup()->hide();
	std::string package = "<span size=\"xx-large\"><b>";
	package += PACKAGE_STRING;
	package += "</b></span>";
	gtk_label_set_markup (GTK_LABEL (get("about_version")), package.c_str());
	GUI::show ("about");
}

// ================================================================================
void AppletGTK::on_hide_about (void)
{
	GUI::hide ("about");
}
