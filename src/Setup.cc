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
#include <glib.h>
#include "Setup.h"
#include "Biff.h"
#include "Mailbox.h"
#include "File.h"
#include "Mh.h"
#include "Maildir.h"
#include "Imap4.h"
#include "Pop3.h"
#include "Apop.h"
#include "Popup.h"
#include "gtk_image_animation.h"


// ===================================================================
// = Global attributes (dirty) =======================================
// ===================================================================
GtkImageAnimation *preview_animation = 0;


// ===================================================================
// = Callbacks "C" binding ===========================================
// ===================================================================
extern "C" {
	void SETUP_on_cancel (GtkWidget *widget, gpointer data)				{((Setup *) data)->on_cancel ();}
	void SETUP_on_save (GtkWidget *widget, gpointer data)				{((Setup *) data)->on_save ();}
	void SETUP_on_quit (GtkWidget *widget, gpointer data)				{((Setup *) data)->on_quit ();}
	void SETUP_on_suspend (GtkWidget *widget, gpointer data)			{((Setup *) data)->on_suspend ();}
	void SETUP_on_tree_select (GtkTreeSelection *sel, gpointer data)	{((Setup *) data)->on_tree_select (sel);}
	void SETUP_on_protocol (GtkWidget *widget, gpointer data)			{((Setup *) data)->on_protocol ();}
	void SETUP_on_sound (GtkWidget *widget, gpointer data)				{((Setup *) data)->on_sound ();}
	void SETUP_on_misc_change (GtkWidget *widget, gpointer data)		{((Setup *) data)->on_misc_change ();}
	void SETUP_on_browse_address (GtkWidget *widget, gpointer data)		{((Setup *) data)->on_browse_address ();}
	void SETUP_on_browse_certificate (GtkWidget *widget, gpointer data)	{((Setup *) data)->on_browse_certificate ();}
	void SETUP_on_browse_sound (GtkWidget *widget, gpointer data)		{((Setup *) data)->on_browse_sound ();}
	void SETUP_on_play_sound (GtkWidget *widget, gpointer data)			{((Setup *) data)->on_play_sound ();}
	void SETUP_on_browse_mail_app (GtkWidget *widget, gpointer data)	{((Setup *) data)->on_browse_mail_app ();}
	void SETUP_on_browse_mail_image (GtkWidget *widget, gpointer data)	{((Setup *) data)->on_browse_mail_image ();}
	void SETUP_on_browse_nomail_image (GtkWidget *widget, gpointer data)	{((Setup *) data)->on_browse_nomail_image ();}
	void SETUP_on_new_mailbox (GtkWidget *widget, gpointer data)		{((Setup *) data)->on_new_mailbox ();}
	void SETUP_on_delete_mailbox (GtkWidget *widget, gpointer data)		{((Setup *) data)->on_delete_mailbox ();}
	void SETUP_on_prev_mailbox (GtkWidget *widget, gpointer data)		{((Setup *) data)->on_prev_mailbox ();}
	void SETUP_on_next_mailbox (GtkWidget *widget, gpointer data)		{((Setup *) data)->on_next_mailbox ();}

	void SETUP_update_preview (GtkWidget *widget, gpointer data) {
		GtkWidget *preview;
		char *filename;
		gboolean have_preview = false;
		preview = GTK_WIDGET (data);
		if (preview_animation == 0)
			preview_animation = new GtkImageAnimation (GTK_IMAGE (preview));
		preview_animation->attach (GTK_IMAGE (preview));
		filename = gtk_file_chooser_get_preview_filename (GTK_FILE_CHOOSER(widget));
		if (filename) {
			have_preview = 	preview_animation->open (filename);
			g_free (filename);	
		}
		else {
			have_preview = false;
		}
		if (have_preview)
				preview_animation->start();
		gtk_file_chooser_set_preview_widget_active (GTK_FILE_CHOOSER(widget), have_preview);
	}
}


// ===================================================================
// = Setup ===========================================================
// ===================================================================
Setup::Setup (Biff *owner, std::string xmlFilename) : GUI (xmlFilename) {
	_owner = owner;
	_current = 0;
}


// ===================================================================
// = ~Setup ==========================================================
// ===================================================================
Setup::~Setup (void) {
}

// ===================================================================
// = create ==========================================================
/**
   Create the interface (GUI::create()) and build the tree menu
   @return true
*/
// ===================================================================
gint Setup::create (void) {
	GUI::create();

	// Hide tabs from the notebook
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK(get("Notebook")), FALSE);

	// Make event box under label to be prelighted (cosmetic only)
	gtk_widget_set_state (get("NotebookEventBox"), GTK_STATE_PRELIGHT);

	// Create the tree on the left
	GtkTreeStore      *	tree;
	GtkWidget         *	treeview;
	GtkTreeViewColumn *	column;
	GtkCellRenderer   *	cell;
	GtkTreeSelection  *	sel;
	GtkTreeIter			top_iter, child_iter;

	// Create a new tree store with 5 cells:
	tree = gtk_tree_store_new (5,
							   GDK_TYPE_PIXBUF,		// Small image displayed directly in the tree entry
							   G_TYPE_STRING,		// Label displayed directly in the tree entry
							   G_TYPE_INT,			// Page index related to this tree entry
							   G_TYPE_STRING,		// Notebook label that will be displayed
							   GDK_TYPE_PIXBUF);	// Big image to be displayed next to notebook label
  
	// Create a tree view for the tree...
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (tree));
	//  ... and we do not want to see headers in
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);

	// Create a column template...
	column = gtk_tree_view_column_new ();
	//  ... first cell is the pixbuf...
	cell = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, cell, FALSE);
	gtk_tree_view_column_set_attributes (column, cell, "pixbuf", 0, NULL);
	// ... second cell is the label
	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, cell, TRUE);
	gtk_tree_view_column_set_attributes (column, cell, "text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	// We put our treeview within the tree frame
	gtk_container_add (GTK_CONTAINER (get("TreeFrame")), treeview);

	// We connect our selection callback
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	g_signal_connect (G_OBJECT (sel), "changed",  G_CALLBACK (SETUP_on_tree_select), this);

	// Now we add branches to the tree
	addTreeBranch (tree, _("Mailbox"), GNUBIFF_DATADIR"/BiffMailbox.png", _("Mailbox configuration"), NULL, &top_iter, 0);
	gtk_tree_selection_select_iter (sel, &top_iter);
	addTreeBranch (tree, _("Options"), GNUBIFF_DATADIR"/BiffOptions.png", _("Options"), &top_iter, &child_iter, 1);
	addTreeBranch (tree, _("Layout"), GNUBIFF_DATADIR"/BiffLayout.png", _("Layout"), NULL, &top_iter, 2);
	addTreeBranch (tree, _("Geometry"), GNUBIFF_DATADIR"/BiffGeometry.png", _("Geometry"), &top_iter, &child_iter, 3);
	addTreeBranch (tree, _("Credits"), GNUBIFF_DATADIR"/BiffCredits.png", _("Credits"), NULL, &top_iter, 4);

	// We expand the tree so all branches are visible...
	gtk_tree_view_expand_all (GTK_TREE_VIEW (treeview));

	// ... and we show it
	gtk_widget_show_all (treeview);

	// We don't need this object anymore
	g_object_unref (G_OBJECT (tree));

	// Make thanks text smaller
	PangoFontDescription *font;
	font = pango_font_description_from_string ("Sans 8");
	gtk_widget_modify_font (get("ThanksText"), font);
	pango_font_description_free (font);

	// Here we attach some data to widgets that cannot contain
	// enough information by themselves. For example the sound button
	// only contains name of the basename of the file (without path), so
	// we need to store fullname somewhere else.
	g_object_set_data (G_OBJECT(get("soundfile")), "_file_", gpointer (0));
	g_object_set_data (G_OBJECT(get("mail_image")), "_animation_",
					   new GtkImageAnimation (GTK_IMAGE(get("mail_image"))));
	g_object_set_data (G_OBJECT(get("nomail_image")), "_animation_",
					   new GtkImageAnimation (GTK_IMAGE(get("nomail_image"))));


	// Now we make some widgets sensitive or not depending on USE_GNOME
#ifdef USE_GNOME
	if (_owner->_mode == GNOME_MODE) {
		gtk_widget_set_sensitive (get("biff_geometry"), FALSE);
		gtk_widget_set_sensitive (get("quit"), FALSE);
	}
#endif

	// If we don't have crypto, we can't use apop
#ifndef HAVE_CRYPTO
	gtk_widget_set_sensitive (get("apop"), FALSE);
#endif

	// If we don't have ssl, we can't use it
#ifndef HAVE_LIBSSL
	gtk_widget_set_sensitive (get("use_ssl"), FALSE);
	gtk_widget_set_sensitive (get("CertificateTable"), FALSE);
#endif

	// That's it
	return true;
}


// ===================================================================
// = update ==========================================================
/**
   update Setup according to _owner
*/
// ===================================================================
void Setup::update (void) {
	// Nothing special, we read settings from _owner and set corresponding
	//  widgets accordingly.
	// Very boring code indeed !
	//
	// Mailbox
	gtk_entry_set_text (GTK_ENTRY (get("name")), _owner->mailbox(_current)->name().c_str());
	std::string protocol = "file";
	switch (_owner->mailbox(_current)->protocol()) {
	case PROTOCOL_MH:
		protocol = "mh";
		break;
	case PROTOCOL_MAILDIR:
		protocol = "maildir";
		break;
	case PROTOCOL_POP3:
		protocol = "pop3";
		break;
	case PROTOCOL_APOP:
		protocol = "apop";
		break;
	case PROTOCOL_IMAP4:
		protocol = "imap4";
		break;
	}

	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get(protocol.c_str())))) 
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get(protocol.c_str())), TRUE);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("use_ssl"))) != _owner->mailbox(_current)->use_ssl())
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("use_ssl")), _owner->mailbox(_current)->use_ssl());

	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("port")), _owner->mailbox(_current)->port());
	gtk_entry_set_text (GTK_ENTRY (get("address")), _owner->mailbox(_current)->address().c_str());
	gtk_entry_set_text (GTK_ENTRY (get("user")), _owner->mailbox(_current)->user().c_str());
	gtk_entry_set_text (GTK_ENTRY (get("folder")), _owner->mailbox(_current)->folder().c_str());
	gtk_entry_set_text (GTK_ENTRY (get("password")), _owner->mailbox(_current)->password().c_str());
	gtk_entry_set_text (GTK_ENTRY (get("certificate")), _owner->mailbox(_current)->certificate().c_str());
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("polltime")), _owner->mailbox(_current)->polltime());


	// Options
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("poptime")), _owner->_poptime);
	std::string sound_type = "nosound";
	switch (_owner->_sound_type) {
	case SOUND_BEEP:
		sound_type = "beep";
		break;
	case SOUND_FILE:
		sound_type = "sound";
		break;
	}
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get(sound_type.c_str())), TRUE);
	gchar *basename = g_path_get_basename (_owner->_sound_file.c_str());
	gtk_label_set_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN(get("soundfile")))), basename);
	g_free (basename);
	// Here, we attach the full soundfile name to the widget, taking care of previous attached data
	gchar *data = (gchar *) g_object_get_data (G_OBJECT(get("soundfile")), "_file_");
	if (data)
		g_free (data);
	data = g_strdup (_owner->_sound_file.c_str());
	g_object_set_data (G_OBJECT(get("soundfile")), "_file_", gpointer (data));
	gtk_entry_set_text (GTK_ENTRY (get("sound_command")), _owner->_sound_command.c_str());
	gtk_range_set_value (GTK_RANGE (get ("volume")), _owner->_sound_volume);  
	if (_owner->_state == STATE_RUNNING) {
		gtk_label_set_markup (GTK_LABEL (get("suspend_label")), _("<span color=\"darkgreen\"><b>Running</b></span>"));
		gtk_image_set_from_stock (GTK_IMAGE (get("suspend_image")), GTK_STOCK_EXECUTE, GTK_ICON_SIZE_BUTTON);
	}
	else {
		gtk_label_set_markup (GTK_LABEL (get("suspend_label")), _("<span color=\"red\"><b>Stopped</b></span>"));
		gtk_image_set_from_stock (GTK_IMAGE (get("suspend_image")), GTK_STOCK_STOP, GTK_ICON_SIZE_BUTTON);
	}
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("max_collected_mail")), _owner->_max_collected_mail);

	// Advanced
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("display_popup")), _owner->_display_popup); 
	gtk_entry_set_text (GTK_ENTRY (get("mail_app")), _owner->_mail_app.c_str());

	// Layout
	GtkImageAnimation *anim;
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("mail_image")), "_animation_");
	anim->open (_owner->_mail_image.c_str());
	anim->start();
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("nomail_image")), "_animation_");
	anim->open (_owner->_nomail_image.c_str());
	anim->start();

	// Color and font
	GdkColor color;
	gdk_color_parse (_owner->_popup_font_color.c_str(), &color);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (get("popup_font_color")), &color);
	gdk_color_parse (_owner->_popup_back_color.c_str(), &color);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (get("popup_back_color")), &color);
	gdk_color_parse (_owner->_applet_font_color.c_str(), &color);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (get("applet_font_color")), &color);
	gtk_font_button_set_font_name (GTK_FONT_BUTTON(get("applet_font")), _owner->_applet_font.c_str());
	gtk_font_button_set_font_name (GTK_FONT_BUTTON(get("popup_font")), _owner->_popup_font.c_str());
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("display_date")), _owner->_display_date); 
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("hide_newmail_image")), _owner->_hide_newmail_image); 
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("hide_nomail_image")), _owner->_hide_nomail_image); 
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("hide_newmail_text")), _owner->_hide_newmail_text); 
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("hide_nomail_text")), _owner->_hide_nomail_text); 
	gtk_entry_set_text (GTK_ENTRY (get("nomail_text")), _owner->_nomail_text.c_str());
	gtk_entry_set_text (GTK_ENTRY (get("newmail_text")), _owner->_newmail_text.c_str());
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("max_sender_size")), _owner->_max_sender_size);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("max_subject_size")), _owner->_max_subject_size);


	// Geometry
	if (_owner->_popup_x_sign == SIGN_PLUS)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("popup_x_plus")), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("popup_x_minus")), TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("popup_x")), _owner->_popup_x);
	if (_owner->_popup_y_sign == SIGN_PLUS)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("popup_y_plus")), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("popup_y_minus")), TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("popup_y")), _owner->_popup_y);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("popup_decorated")), _owner->_popup_decorated); 

	if (_owner->_biff_x_sign == SIGN_PLUS)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("biff_x_plus")), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("biff_x_minus")), TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("biff_x")), _owner->_biff_x);
	if (_owner->_biff_y_sign == SIGN_PLUS)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("biff_y_plus")), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("biff_y_minus")), TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("biff_y")), _owner->_biff_y);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("biff_decorated")), _owner->_biff_decorated); 

	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("max_header_display")), _owner->_max_header_display);


	gtk_widget_set_sensitive (get("network"), TRUE);
	gtk_widget_set_sensitive (get("port_all"), TRUE);
	gtk_widget_set_sensitive (get("address_browse"), FALSE);
#ifdef HAVE_LIBSSL
	gtk_widget_set_sensitive (get("use_ssl"), TRUE);
	gtk_widget_set_sensitive (get("CertificateTable"), TRUE);
#else
	gtk_widget_set_sensitive (get("use_ssl"), FALSE);
	gtk_widget_set_sensitive (get("CertificateTable"), FALSE);
#endif
  
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("file"))) ||
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("mh"))) ||
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("maildir")))) {
		gtk_widget_set_sensitive (get("network"), FALSE);
		gtk_widget_set_sensitive (get("port_all"), FALSE);
		gtk_widget_set_sensitive (get("address_browse"), TRUE);
		gtk_widget_set_sensitive (get("use_ssl"), FALSE);
		gtk_widget_set_sensitive (get("CertificateTable"), FALSE);
	}

	gtk_widget_set_sensitive (get("folder"), FALSE);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("maildir"))) ||
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("imap4")))) {
		gtk_widget_set_sensitive (get("folder"), TRUE);
	}

	gtk_widget_set_sensitive (get("PrevMailbox"), TRUE);
	if (_current == 0)
		gtk_widget_set_sensitive (get("PrevMailbox"), FALSE);
	gtk_widget_set_sensitive (get("NextMailbox"), TRUE);
	if (_current == int(_owner->mailboxes()-1))
		gtk_widget_set_sensitive (get("NextMailbox"), FALSE);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("use_ssl"))))
		gtk_image_set_from_file (GTK_IMAGE(get("ssl_image")), GNUBIFF_DATADIR"/stock-ssl.png");
	else
		gtk_image_set_from_file (GTK_IMAGE(get("ssl_image")), GNUBIFF_DATADIR"/stock-nossl.png");

	// Now we call some callbacks to have some widgets sensitive state updated
	on_sound ();
	on_misc_change ();
}


// ===================================================================
// = updateOwner =====================================================
/**
   update _owner according to widgets content
*/
// ===================================================================
void Setup::updateOwner (void) {
	//
	// Nothing special, we read widget content and set _owner (Biff)
	//  accordingly.
	// Still very boring code...
	//
	// Mailbox
	_owner->mailbox(_current)->name() = gtk_entry_get_text (GTK_ENTRY (get("name")));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("file")))) {
		if (_owner->mailbox(_current)->protocol() != PROTOCOL_FILE) {
			Mailbox *mailbox = new File (_owner->mailbox(_current));
			delete _owner->mailbox(_current);
			_owner->mailbox(_current) = mailbox;
		}
	}
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("mh")))) {
		if (_owner->mailbox(_current)->protocol() != PROTOCOL_MH) {
			Mailbox *mailbox = new Mh (_owner->mailbox(_current));
			delete _owner->mailbox(_current);
			_owner->mailbox(_current) = mailbox;
		}
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("maildir")))) {
		if (_owner->mailbox(_current)->protocol() != PROTOCOL_MAILDIR) {
			Mailbox *mailbox = new Maildir (_owner->mailbox(_current));
			delete _owner->mailbox(_current);
			_owner->mailbox(_current) = mailbox;
		}
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("pop3")))) {
		if (_owner->mailbox(_current)->protocol() != PROTOCOL_POP3) {
			Mailbox *mailbox = new Pop3 (_owner->mailbox(_current));
			delete _owner->mailbox(_current);
			_owner->mailbox(_current) = mailbox;
		}
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("apop")))) {
		if (_owner->mailbox(_current)->protocol() != PROTOCOL_APOP) {
			Mailbox *mailbox = new Apop (_owner->mailbox(_current));
			delete _owner->mailbox(_current);
			_owner->mailbox(_current) = mailbox;
		}
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("imap4")))) {
		if (_owner->mailbox(_current)->protocol() != PROTOCOL_IMAP4) {
			Mailbox *mailbox = new Imap4 (_owner->mailbox(_current));
			delete _owner->mailbox(_current);
			_owner->mailbox(_current) = mailbox;
		}
	}
	_owner->mailbox(_current)->port() = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(get("port")));
	_owner->mailbox(_current)->use_ssl() = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("use_ssl")));
	_owner->mailbox(_current)->address() =gtk_entry_get_text (GTK_ENTRY (get("address")));
	_owner->mailbox(_current)->user() = gtk_entry_get_text (GTK_ENTRY (get("user")));
	_owner->mailbox(_current)->password() =  gtk_entry_get_text (GTK_ENTRY (get("password")));
	_owner->mailbox(_current)->certificate() =  gtk_entry_get_text (GTK_ENTRY (get("certificate")));
	_owner->mailbox(_current)->folder() =  gtk_entry_get_text (GTK_ENTRY (get("folder")));
	_owner->mailbox(_current)->polltime() =  gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(get("polltime")));

	// Options
	_owner->_poptime =  gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(get("poptime")));
	_owner->_sound_type = SOUND_NONE;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("beep"))))
		_owner->_sound_type = SOUND_BEEP;
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("sound"))))
		_owner->_sound_type = SOUND_FILE;
	gchar *data = (gchar *) g_object_get_data (G_OBJECT(get("soundfile")), "_file_");
	_owner->_sound_file = data;
	_owner->_sound_command =  gtk_entry_get_text (GTK_ENTRY (get("sound_command")));
	_owner->_sound_volume = (int) (gtk_range_get_value (GTK_RANGE (get ("volume"))));
	_owner->_max_collected_mail = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(get("max_collected_mail")));

	// Advanced
	_owner->_display_popup = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("display_popup")));
	_owner->_mail_app =  gtk_entry_get_text (GTK_ENTRY (get("mail_app")));


	// Layout
	GtkImageAnimation *anim;
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("mail_image")), "_animation_");
	_owner->_mail_image = anim->filename();
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("nomail_image")), "_animation_");
	_owner->_nomail_image = anim->filename();
	_owner->_display_date = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("display_date")));
	_owner->_hide_nomail_image = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("hide_nomail_image")));
	_owner->_hide_newmail_image = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("hide_newmail_image")));
	_owner->_hide_nomail_text = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("hide_nomail_text")));
	_owner->_hide_newmail_text = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("hide_newmail_text")));
	_owner->_nomail_text =  gtk_entry_get_text (GTK_ENTRY (get("nomail_text")));
	_owner->_newmail_text =  gtk_entry_get_text (GTK_ENTRY (get("newmail_text")));
	_owner->_max_sender_size =  gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(get("max_sender_size")));
	_owner->_max_subject_size =  gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(get("max_subject_size")));


	// Geometry
	_owner->_popup_x_sign = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("popup_x_plus")));
	_owner->_popup_x = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(get("popup_x")));
	_owner->_popup_y_sign = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("popup_y_plus")));
	_owner->_popup_y = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(get("popup_y")));
	_owner->_popup_decorated = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("popup_decorated")));
	_owner->_biff_x_sign = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("biff_x_plus")));
	_owner->_biff_x = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(get("biff_x")));
	_owner->_biff_y_sign = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("biff_y_plus")));
	_owner->_biff_y = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(get("biff_y")));
	_owner->_biff_decorated = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("biff_decorated")));
	_owner->_max_header_display = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(get("max_header_display")));

  
	// Font & color
	GdkColor color;
	char buffer[16];
	gtk_color_button_get_color (GTK_COLOR_BUTTON (get("popup_font_color")),  &color);
	sprintf (buffer, "#%.2x%.2x%.2x", color.red/256, color.green/256, color.blue/256);
	_owner->_popup_font_color = buffer;
	gtk_color_button_get_color (GTK_COLOR_BUTTON (get("popup_back_color")),  &color);
	sprintf (buffer, "#%.2x%.2x%.2x", color.red/256, color.green/256, color.blue/256);
	_owner->_popup_back_color = buffer;
	gtk_color_button_get_color (GTK_COLOR_BUTTON (get("applet_font_color")),  &color);
	sprintf (buffer, "#%.2x%.2x%.2x", color.red/256, color.green/256, color.blue/256);
	_owner->_applet_font_color = buffer;

	_owner->_applet_font = gtk_font_button_get_font_name (GTK_FONT_BUTTON(get("applet_font")));
	_owner->_popup_font = gtk_font_button_get_font_name (GTK_FONT_BUTTON(get("popup_font")));
}


// ===================================================================
// = on_delete =======================================================
/**
   Until we're able to properly catch the hide event on the image, we
   need to manually start and stop animation.
*/
// ===================================================================
void Setup:: show (std::string name) {
	if (_xml) {
		gtk_widget_show (get(name));
		GtkImageAnimation *anim;
		anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("mail_image")), "_animation_");
		anim->start();
		anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("nomail_image")), "_animation_");
		anim->start();
	}
}
void Setup:: hide (std::string name) {
	if (_xml) {
		gtk_widget_hide (get(name));
		GtkImageAnimation *anim;
		anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("mail_image")), "_animation_");
		anim->stop();
		anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("nomail_image")), "_animation_");
		anim->stop();
	}
}

// ===================================================================
// = on_delete =======================================================
/**
   Callback on delete event: all parameters are dropped and the
   setting dialog is hidden.
   @param widget where event occured
   @param event is the delete event
*/
// ===================================================================
gboolean Setup::on_delete (GtkWidget *widget,  GdkEvent *event) {
	hide();
	update();
	_owner->applet()->update();
	_owner->applet()->show();
	if (_owner->_state == STATE_RUNNING)
		_owner->applet()->watch_now();

	GtkImageAnimation *anim;
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("mail_image")), "_animation_");
	anim->stop();
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("nomail_image")), "_animation_");
	anim->stop();

	return true;
}


// ===================================================================
// = on_cancel =======================================================
/**
   Callback on cancel button: all parameters are dropped and the
   setting dialog is hidden
*/
// ===================================================================
void Setup::on_cancel (void) {
	hide();
	update ();
	_owner->applet()->update();
	_owner->applet()->show();
	if (_owner->_state == STATE_RUNNING)
		_owner->applet()->watch_now();

	GtkImageAnimation *anim;
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("mail_image")), "_animation_");
	anim->stop();
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("nomail_image")), "_animation_");
	anim->stop();
}


// ===================================================================
// = on_save =========================================================
/**
   Callback on save button: all parameters are updated from GUI and
   saved
*/
// ===================================================================
void Setup::on_save (void) {
	hide();
	updateOwner();
	_owner->save ();
	_owner->applet()->update();
	_owner->applet()->show();
	if (_owner->_state == STATE_RUNNING)
		_owner->applet()->watch_now();

	GtkImageAnimation *anim;
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("mail_image")), "_animation_");
	anim->stop();
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("nomail_image")), "_animation_");
	anim->stop();	
}


// ===================================================================
// = on_quit =========================================================
/**
   Callback on quit button: exit gnubiff
*/
// ===================================================================
void Setup::on_quit (void) {
	_owner->save ();
	gtk_main_quit();
}


// ===================================================================
// = on_suspend ======================================================
/**
   Callback on suspend button: switch to supend or continue state
   (manual or automatic checking)
*/
// ===================================================================
void Setup::on_suspend (void) {
	if (_owner->_state == STATE_RUNNING) {
		gtk_label_set_markup (GTK_LABEL (get("suspend_label")), _("<span color=\"red\"><b>Stopped</b></span>"));
		gtk_image_set_from_stock (GTK_IMAGE (get("suspend_image")), GTK_STOCK_STOP, GTK_ICON_SIZE_BUTTON);
		_owner->_state = STATE_STOPPED;
	}
	else {
		gtk_label_set_markup (GTK_LABEL (get("suspend_label")), _("<span color=\"darkgreen\"><b>Running</b></span>"));
		gtk_image_set_from_stock (GTK_IMAGE (get("suspend_image")), GTK_STOCK_EXECUTE, GTK_ICON_SIZE_BUTTON);
		_owner->_state = STATE_RUNNING;
	}
}

// ===================================================================
// = on_protocol =====================================================
/**
   Callback on protocol (any) button: when a protocol button is
   pressed, we update user and password sensitive state
*/
// ===================================================================
void Setup::on_protocol (void) {
	gtk_widget_set_sensitive (get("network"), TRUE);
	gtk_widget_set_sensitive (get("port_all"), TRUE);
	gtk_widget_set_sensitive (get("address_browse"), FALSE);
#ifdef HAVE_LIBSSL
	gtk_widget_set_sensitive (get("use_ssl"), TRUE);
	gtk_widget_set_sensitive (get("CertificateTable"), TRUE);
#else
	gtk_widget_set_sensitive (get("use_ssl"), FALSE);
	gtk_widget_set_sensitive (get("CertificateTable"), FALSE);
#endif
  
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("file"))) ||
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("mh"))) ||
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("maildir")))) {
		gtk_widget_set_sensitive (get("network"), FALSE);
		gtk_widget_set_sensitive (get("port_all"), FALSE);
		gtk_widget_set_sensitive (get("address_browse"), TRUE);
		gtk_widget_set_sensitive (get("use_ssl"), FALSE);
		gtk_widget_set_sensitive (get("CertificateTable"), FALSE);
	}

	gtk_widget_set_sensitive (get("folder"), FALSE);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("maildir"))) ||
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("imap4")))) {
		gtk_widget_set_sensitive (get("folder"), TRUE);
	}

	gtk_widget_set_sensitive (get("PrevMailbox"), TRUE);
	if (_current == 0)
		gtk_widget_set_sensitive (get("PrevMailbox"), FALSE);
	gtk_widget_set_sensitive (get("NextMailbox"), TRUE);
	if (_current == int(_owner->mailboxes()-1))
		gtk_widget_set_sensitive (get("NextMailbox"), FALSE);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("use_ssl"))))
		gtk_image_set_from_file (GTK_IMAGE(get("ssl_image")), GNUBIFF_DATADIR"/stock-ssl.png");
	else
		gtk_image_set_from_file (GTK_IMAGE(get("ssl_image")), GNUBIFF_DATADIR"/stock-nossl.png");

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("maildir")))) {
		gtk_entry_set_text (GTK_ENTRY(get("folder")), DEFAULT_MAILDIR_FOLDER);
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("imap4")))) {
		gtk_entry_set_text (GTK_ENTRY(get("folder")), DEFAULT_IMAP4_FOLDER);
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("use_ssl"))))
			gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("port")), DEFAULT_SSL_IMAP4_PORT);
		else
			gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("port")), DEFAULT_IMAP4_PORT);
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("pop3")))) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("use_ssl"))))
			gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("port")), DEFAULT_SSL_POP3_PORT);
		else
			gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("port")), DEFAULT_POP3_PORT);
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("apop")))) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("use_ssl"))))
			gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("port")), DEFAULT_SSL_POP3_PORT);
		else
			gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("port")), DEFAULT_POP3_PORT);
	}

	if(!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("use_ssl")))){
		gtk_widget_set_sensitive (get("CertificateTable"), FALSE);
	}
}

// ===================================================================
// = on_sound ========================================================
/**
   Callback on sound (any) button: update mailfile, play and volume
   sensitive state
*/
// ===================================================================
void Setup::on_sound (void) {
	gtk_widget_set_sensitive (get("volume_all"), TRUE);
	gtk_widget_set_sensitive (get("soundfile"), TRUE);
	gtk_widget_set_sensitive (get("sound_command_expander"), TRUE);
	gtk_widget_set_sensitive (get("soundplay"), TRUE);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("nosound")))) {
		gtk_widget_set_sensitive (get("volume_all"), FALSE);
		gtk_widget_set_sensitive (get("soundfile"), FALSE);
		gtk_widget_set_sensitive (get("soundfilelabel"), FALSE);
		gtk_widget_set_sensitive (get("soundplay"), FALSE);
		gtk_widget_set_sensitive (get("sound_command_expander"), FALSE);
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("beep")))) {
		gtk_widget_set_sensitive (get("volume_all"), FALSE);
		gtk_widget_set_sensitive (get("soundfile"), FALSE);
		gtk_widget_set_sensitive (get("soundfilelabel"), FALSE);
		gtk_widget_set_sensitive (get("sound_command_expander"), FALSE);
	}
}


// ===================================================================
// = on_misc_change ==================================================
/**
   Callback on misc. changes
*/
// ===================================================================
void Setup::on_misc_change (void) {
    gtk_widget_set_sensitive (get("popup_geometry_table"), TRUE);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("popup_no_geometry"))))
		gtk_widget_set_sensitive (get("popup_geometry_table"), FALSE);
#ifndef USE_GNOME
	gtk_widget_set_sensitive (get("biff_geometry_table"), TRUE);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("biff_no_geometry"))))
		gtk_widget_set_sensitive (get("biff_geometry_table"), FALSE);
#else
	if (_owner->_mode == GTK_MODE) {
		gtk_widget_set_sensitive (get("biff_geometry_table"), TRUE);
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("biff_no_geometry"))))
			gtk_widget_set_sensitive (get("biff_geometry_table"), FALSE);
	}
#endif

    gtk_widget_set_sensitive (get("MailEventBox"), TRUE);
    gtk_widget_set_sensitive (get("NoMailEventBox"), TRUE);
	gtk_widget_set_sensitive (get("nomail_text"), TRUE);
	gtk_widget_set_sensitive (get("newmail_text"), TRUE);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("hide_newmail_image"))))
		gtk_widget_set_sensitive (get("MailEventBox"), FALSE);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("hide_nomail_image"))))
		gtk_widget_set_sensitive (get("NoMailEventBox"), FALSE);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("hide_nomail_text"))))
		gtk_widget_set_sensitive (get("nomail_text"), FALSE);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("hide_newmail_text"))))
		gtk_widget_set_sensitive (get("newmail_text"), FALSE);

    gtk_widget_set_sensitive (get("popup_layout"), TRUE);
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("display_popup"))))
		gtk_widget_set_sensitive (get("popup_layout"), FALSE);

}


// ===================================================================
// = on_browse_address ===============================================
/**
   Callback on address browse button: open a file selector to browse
   for a file
*/
// ===================================================================
void Setup::on_browse_address (void) {
	GtkWidget *chooser = 
		gtk_file_chooser_dialog_new ("Browse for mailbox file/directory", 0,
									 GTK_FILE_CHOOSER_ACTION_OPEN,
									 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
									 NULL);
	gtk_window_set_position (GTK_WINDOW (chooser), GTK_WIN_POS_CENTER);

	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser),
								   gtk_entry_get_text (GTK_ENTRY (get("address"))));
	gint result = gtk_dialog_run (GTK_DIALOG (chooser));
	if (result == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
		gtk_entry_set_text (GTK_ENTRY (get("address")), filename);
		g_free (filename);
	}
	gtk_widget_destroy (chooser);
}

// ===================================================================
// = on_browse_certificate============================================
/**
   Callback on certificate browse button: open a file selector to 
   browse for a file
*/
// ===================================================================
void Setup::on_browse_certificate (void) {
	GtkWidget *chooser = 
		gtk_file_chooser_dialog_new ("Browse for server certificate", 0,
									 GTK_FILE_CHOOSER_ACTION_OPEN,
									 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
									 NULL);
	gtk_window_set_position (GTK_WINDOW (chooser), GTK_WIN_POS_CENTER);
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser),
								   gtk_entry_get_text (GTK_ENTRY (get("address"))));
	gint result = gtk_dialog_run (GTK_DIALOG (chooser));
	if (result == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
		gtk_entry_set_text (GTK_ENTRY (get("certificate")), filename);
		g_free (filename);
	}
	gtk_widget_destroy (chooser);
}

// ===================================================================
// = on_browse_sound =================================================
/**
   Callback on sound browse button: open a file selector to browse for
   a soundfile
*/
// ===================================================================
void Setup::on_browse_sound (void) {
	GtkWidget *chooser = 
		gtk_file_chooser_dialog_new ("Open a soundfile", 0,
									 GTK_FILE_CHOOSER_ACTION_OPEN,
									 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
									 NULL);
	gtk_window_set_position (GTK_WINDOW (chooser), GTK_WIN_POS_CENTER);
	gchar *data = (gchar *) g_object_get_data (G_OBJECT(get("soundfile")), "_file_");
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), data);
	gint result = gtk_dialog_run (GTK_DIALOG (chooser));
	if (result == GTK_RESPONSE_ACCEPT) {
		g_free (data);
		data = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
		g_object_set_data (G_OBJECT(get("soundfile")), "_file_", gpointer (data));
		gchar *basename = g_path_get_basename (data);
		gtk_label_set_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN(get("soundfile")))), basename);
		g_free (basename);
	}
	gtk_widget_destroy (chooser);
}


// ===================================================================
// = on_play_sound ===================================================
/**
   Callback on sound play button: play current soundfile or beep
*/
// ===================================================================
void Setup::on_play_sound (void) {
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("beep"))))
		gdk_beep ();    
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("sound")))) {
		gchar *data = (gchar *) g_object_get_data (G_OBJECT(get("soundfile")), "_file_");
		std::stringstream s;
		s << gfloat(gtk_range_get_value (GTK_RANGE (get ("volume"))))/100.0;
		std::string command = gtk_entry_get_text (GTK_ENTRY(get("sound_command")));
		guint i;
		while ((i = command.find ("%s")) != std::string::npos) {
			command.erase (i, 2);
			std::string filename = std::string("\"") + std::string(data) + std::string("\"");
			command.insert(i, filename);
		}
		while ((i = command.find ("%v")) != std::string::npos) {
			command.erase (i, 2);
			command.insert(i, s.str());
		}
		command += " &";
		system (command.c_str());
	}
}


// ===================================================================
// = on_browse_mail_app ==============================================
/**
   Callback on mail_app browse button: open a file selector to browse
   for a mail app
*/
// ===================================================================
void Setup::on_browse_mail_app (void) {
	GtkWidget *chooser = 
		gtk_file_chooser_dialog_new ("Open a mail application", 0,
									 GTK_FILE_CHOOSER_ACTION_OPEN,
									 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
									 NULL);
	gtk_window_set_position (GTK_WINDOW (chooser), GTK_WIN_POS_CENTER);
	gchar *data = (gchar *) g_object_get_data (G_OBJECT(get("mail_app")), "_file_");
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), data);
	gint result = gtk_dialog_run (GTK_DIALOG (chooser));
	if (result == GTK_RESPONSE_ACCEPT) {
		g_free (data);
		data = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
		g_object_set_data (G_OBJECT(get("mail_app")), "_file_", gpointer (data));
		gchar *basename =  g_path_get_basename (data);
		gtk_entry_set_text (GTK_ENTRY (get("mail_app")), basename);
		g_free (basename);
	}
	gtk_widget_destroy (chooser);
}

// ===================================================================
// = on_browse_mail_image ============================================
/**
   Callback on mail_image browse button: open a file selector to
   browse for a mail image
*/
// ===================================================================
void Setup::on_browse_mail_image (void) {
	GtkImageAnimation *anim;
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("mail_image")), "_animation_");
	GtkWidget *preview = gtk_image_new ();
	GtkWidget *chooser = 
		gtk_file_chooser_dialog_new ("Open a mail image or animation", 0,
									 GTK_FILE_CHOOSER_ACTION_OPEN,
									 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
									 NULL);
	gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (chooser), preview);
	g_signal_connect (chooser, "update-preview",  G_CALLBACK (SETUP_update_preview), preview);
	gtk_window_set_position (GTK_WINDOW (chooser), GTK_WIN_POS_CENTER);
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), anim->filename().c_str());
	gint result = gtk_dialog_run (GTK_DIALOG (chooser));
	if (result == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
		anim->open (filename);
		g_free (filename);
		preview_animation->stop();
		gtk_widget_destroy (chooser);
		anim->start();
	}
	else {
		preview_animation->stop();
		gtk_widget_destroy (chooser);
	}
}

// ===================================================================
// = on_browse_nomail_image ==========================================
/**
   Callback on nomail_image browse button: open a file selector to
   browse for a nomail image
*/
// ===================================================================
void Setup::on_browse_nomail_image (void) {
	GtkImageAnimation *anim;
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("nomail_image")), "_animation_");
	GtkWidget *preview = gtk_image_new ();
	GtkWidget *chooser = 
		gtk_file_chooser_dialog_new ("Open a nomail image or animation", 0,
									 GTK_FILE_CHOOSER_ACTION_OPEN,
									 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
									 NULL);
	gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (chooser), preview);
	g_signal_connect (chooser, "update-preview",  G_CALLBACK (SETUP_update_preview), preview);
	gtk_window_set_position (GTK_WINDOW (chooser), GTK_WIN_POS_CENTER);
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), anim->filename().c_str());
	gint result = gtk_dialog_run (GTK_DIALOG (chooser));
	if (result == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
		anim->open (filename);
		g_free (filename);
		preview_animation->stop();
		gtk_widget_destroy (chooser);
		anim->start();
	}
	else {
		preview_animation->stop();
		gtk_widget_destroy (chooser);
	}
}


// ===================================================================
// = on_new_mailbox ==================================================
/**
   Add a new mailbox
*/
// ===================================================================
void Setup::on_new_mailbox (void) {
	_owner->mailbox().push_back (new File (_owner));
	_current = _owner->mailboxes()-1;
	update();
}


// ===================================================================
// = on_delete_mailbox ===============================================
/**
   Delete a mailbox
*/
// ===================================================================
void Setup::on_delete_mailbox (void) {
	if (_owner->mailboxes() > 1) {
		for(std::vector<Mailbox *>::iterator i  = _owner->mailbox().begin(); i != _owner->mailbox().end(); i++ ) {
			if (*i ==  _owner->mailbox(_current)) {
				_owner->mailbox().erase(i);
				break;
			}
		}
		if (_current > int(_owner->mailboxes()-1))
			_current = _owner->mailboxes()-1;
		update();
	}
}


// ===================================================================
// = on_prev_mailbox =================================================
/**
   Go to previous mailbox
*/
// ===================================================================
void Setup::on_prev_mailbox (void) {
	if (_current > 0) {
		updateOwner();
		_current--;
		update();
	}
}


// ===================================================================
// = on_next_mailbox =================================================
/**
   Go to next mailbox
*/
// ===================================================================
void Setup::on_next_mailbox (void) {
	if (_current < int(_owner->mailboxes()-1)) {
		updateOwner();
		_current++;
		update();
	}
}


// ===================================================================
// = on_tree_select ==================================================
/**
   Callback on tree selection: display the corresponding notebook page
   and update title and icon above the page
   @param sel is the selection
*/
// ===================================================================
void Setup::on_tree_select (GtkTreeSelection *sel)  {
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GValue        val = {0,};

	if (!gtk_tree_selection_get_selected (sel, &model, &iter))
		return;
	gtk_tree_model_get_value (model, &iter, 3, &val);

	std::string label = "<span size=\"x-large\"><b>";
	label += g_value_get_string (&val);
	label += "</b></span>";
	gtk_label_set_markup (GTK_LABEL (get("NotebookLabel")),  label.c_str());

	g_value_unset (&val);
	gtk_tree_model_get_value (model, &iter, 4, &val);
	gtk_image_set_from_pixbuf (GTK_IMAGE (get("NotebookImage")), (GdkPixbuf *) g_value_get_object (&val));
	g_value_unset (&val);
	gtk_tree_model_get_value (model, &iter, 2, &val);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (get("Notebook")), g_value_get_int (&val));
	g_value_unset (&val);
}


// ===================================================================
// = addTreeBranch ===================================================
/**
   Add a new setting to the tree 
   @param tree is the tree store
   @param notebook_label is the notebook page label
   @param icon is the icon for both tree and notebook
   @param tree_label is the new tree label
   @param parent is the parent node
   @param iter is the current iteration (for possible children)
   @param page_index is the index of corresponding notebook page
*/
// ===================================================================
void
Setup::addTreeBranch (GtkTreeStore *tree,
					  gchar *tree_label,
					  const gchar *icon,
					  gchar *notebook_label,
					  GtkTreeIter *parent,
					  GtkTreeIter *iter,
					  gint page_index) {
	GdkPixbuf   *pixbuf       = NULL;
	GdkPixbuf   *big_pixbuf   = NULL;
	GdkPixbuf   *small_pixbuf = NULL;

	if (icon)  {
		if (g_file_test (icon, G_FILE_TEST_IS_REGULAR))
			pixbuf = gdk_pixbuf_new_from_file (icon, NULL);
		else
			pixbuf = NULL;
		if (pixbuf) {
			big_pixbuf = gdk_pixbuf_scale_simple (pixbuf, 48, 48, GDK_INTERP_BILINEAR);
			small_pixbuf = gdk_pixbuf_scale_simple (pixbuf, 18, 18, GDK_INTERP_BILINEAR);
		}
	}
	gtk_tree_store_append (tree, iter, parent);
	gtk_tree_store_set (tree, iter,
						0, small_pixbuf,
						1, tree_label,
						2, page_index,
						3, notebook_label,
						4, big_pixbuf,
						-1);
	if (pixbuf)
		g_object_unref (pixbuf);
	if (small_pixbuf)
		g_object_unref (small_pixbuf);
	if (big_pixbuf)
		g_object_unref (big_pixbuf);
}
