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

#include <sstream>
#include "ui-preferences.h"
#include "ui-properties.h"
#include "ui-applet.h"
#include "gtk_image_animation.h"
#include "biff.h"
#include "mailbox.h"
#include "nls.h"

/* "C" bindings */
extern "C" {
	void PREFERENCES_on_add (GtkWidget *widget,
							 gpointer data)
	{
		PREFERENCES(data)->on_add (widget);
	}

	void PREFERENCES_on_remove (GtkWidget *widget,
								gpointer data)
	{
		PREFERENCES(data)->on_remove (widget);
	}

	void PREFERENCES_on_properties (GtkWidget *widget,
									gpointer data)
	{
		PREFERENCES(data)->on_properties (widget);
	}

	void PREFERENCES_on_stop (GtkWidget *widget,
							  gpointer data)
	{
		PREFERENCES(data)->on_stop (widget);
	}

	void PREFERENCES_on_browse_sound (GtkWidget *widget,
									 gpointer data)
	{
		PREFERENCES(data)->on_browse_sound (widget);
	}

	void PREFERENCES_on_test_sound (GtkWidget *widget,
									gpointer data)
	{
		PREFERENCES(data)->on_test_sound (widget);
	}

	void PREFERENCES_on_browse_mailapp (GtkWidget *widget,
										gpointer data)
	{
		PREFERENCES(data)->on_browse_mailapp (widget);
	}

	void PREFERENCES_on_browse_newmail_image (GtkWidget *widget,
											  gpointer data)
	{
		PREFERENCES(data)->on_browse_newmail_image (widget);
	}

	void PREFERENCES_on_browse_nomail_image (GtkWidget *widget,
											 gpointer data)
	{
		PREFERENCES(data)->on_browse_nomail_image (widget);
	}
	
	void PREFERENCES_on_selection_changed (GtkTreeSelection *selection,
										   gpointer data)
	{
		PREFERENCES(data)->on_selection (selection);
	}
}



Preferences::Preferences (Biff *biff) : GUI (GNUBIFF_DATADIR"/preferences.glade")
{
	biff_ = biff;
	properties_ = new Properties (this);
	properties_->create ();
	added_ = 0;
}


Preferences::~Preferences (void)
{
}

gint
Preferences::create (void)
{
	GUI::create();

	// Mailboxes list
	GtkListStore *store = gtk_list_store_new (N_COLUMNS,
											  G_TYPE_INT,
											  G_TYPE_STRING,
											  G_TYPE_STRING,
											  G_TYPE_STRING,
											  G_TYPE_STRING,
											  G_TYPE_STRING,
											  G_TYPE_STRING,
											  G_TYPE_INT);
	GtkTreeView *view = GTK_TREE_VIEW (get("mailboxes_treeview"));
	gtk_tree_view_set_model (view, GTK_TREE_MODEL(store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (view), TRUE);

	GtkTreeViewColumn *column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Mailbox"));
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_MAILBOX);

	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "stock-id", COLUMN_MAILBOX_ICON);

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "stock-id", COLUMN_SSL_ICON);
  
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text", COLUMN_MAILBOX);
	gtk_tree_view_column_add_attribute (column, renderer, "foreground", COLUMN_FOREGROUND);
  
	gtk_tree_view_append_column (view, column);

	column = gtk_tree_view_column_new_with_attributes (_("Format"),
													   gtk_cell_renderer_text_new(),
													   "text", COLUMN_FORMAT,
													   "foreground", COLUMN_FOREGROUND,
													   NULL);
	gtk_tree_view_column_set_resizable(column, FALSE);
	gtk_tree_view_column_set_sort_column_id(column, COLUMN_FORMAT);
	gtk_tree_view_append_column (view, column);

	column = gtk_tree_view_column_new_with_attributes (_("Status"),
													   gtk_cell_renderer_pixbuf_new (),
													   "stock-id", COLUMN_STATUS,
													   NULL);
	gtk_tree_view_column_set_resizable(column, FALSE);
	gtk_tree_view_column_set_sort_column_id(column, COLUMN_STATUS);
	gtk_tree_view_append_column (view, column);

	column = gtk_tree_view_column_new_with_attributes (_("Poll time"),
													   gtk_cell_renderer_text_new(),
													   "text", COLUMN_POLLTIME,
													   "foreground", COLUMN_FOREGROUND,
													   NULL);
	gtk_tree_view_column_set_resizable (column, FALSE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_POLLTIME);
	gtk_tree_view_append_column (view, column);

	gtk_tree_view_set_search_column (view, COLUMN_MAILBOX);
  
	GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT(selection), "changed",
					  G_CALLBACK(PREFERENCES_on_selection_changed), this);

	// Selection label
	gtk_label_set_text (GTK_LABEL(get ("selection")), _("No mailbox selected"));

	// Animations
	g_object_set_data (G_OBJECT(get("newmail_image")), "_animation_",
					   new GtkImageAnimation (GTK_IMAGE(get("newmail_image"))));
	g_object_set_data (G_OBJECT(get("nomail_image")), "_animation_",
					   new GtkImageAnimation (GTK_IMAGE(get("nomail_image"))));
	return true;
}

void
Preferences::show (std::string name)
{
	if (!xml_)
		return;
	synchronize ();
	gtk_widget_show (get(name));
	// Stop animation
	GtkImageAnimation *anim;
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("newmail_image")), "_animation_");
	anim->start();
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("nomail_image")), "_animation_");
	anim->start();
}

void Preferences:: hide (std::string name) {
	if (!xml_)
		return;
	gtk_widget_hide (get(name));
	// Start animation
	GtkImageAnimation *anim;
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("newmail_image")), "_animation_");
	anim->stop();
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("nomail_image")), "_animation_");
	anim->stop();
}


void
Preferences::synchronize (class Mailbox *mailbox, GtkListStore *store, GtkTreeIter *iter)
{
	if (mailbox) {
		std::string stock_mailbox = "gtk-network";
		std::string stock_status  = "gtk-dialog-error";

		if (mailbox->is_local())
			stock_mailbox = "gtk-home";
		if (mailbox->status() == MAILBOX_UNKNOWN)
			stock_status = "gtk-dialog-warning";
		else if (mailbox->status() == MAILBOX_BLOCKED)
			stock_status = "gtk-stop";
		else if (mailbox->status() != MAILBOX_ERROR)
			stock_status = "gtk-ok";
	

		std::string format;
		switch (mailbox->protocol()) {
		case PROTOCOL_FILE:
			format = "file";
			break;
		case PROTOCOL_MH:
			format = "mh";
			break;
		case PROTOCOL_MAILDIR:
			format = "maildir";
			break;
		case PROTOCOL_IMAP4:
			format = "imap4";
			break;
		case PROTOCOL_POP3:
			format = "pop3";
			break;
		case PROTOCOL_APOP:
			format = "apop";
			break;
		default:
			format = "-";
			break;
		}

		gtk_list_store_set (store, iter,
							COLUMN_UIN, mailbox->uin(),
							COLUMN_MAILBOX_ICON, stock_mailbox.c_str(),
							COLUMN_MAILBOX, mailbox->name().c_str(),
							COLUMN_FOREGROUND, "black",
							COLUMN_FORMAT, format.c_str(),
							COLUMN_STATUS, stock_status.c_str(),
							COLUMN_POLLTIME, mailbox->polltime(),
							-1);

		if ((!mailbox->is_local()) && (mailbox->use_ssl() || mailbox->protocol() == PROTOCOL_APOP)) {
			gtk_list_store_set (store, iter,
								COLUMN_SSL_ICON, "gtk-dialog-authentication",
								-1);
		}
		else if (!mailbox->is_local()) {
			gtk_list_store_set (store, iter,
								COLUMN_SSL_ICON, "",
								COLUMN_FOREGROUND, "red",
								-1);
		}
	}
}

void
Preferences::synchronize (void)
{
	// Mailboxes list
	GtkTreeView  *view  = GTK_TREE_VIEW (get("mailboxes_treeview"));
	GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (view));
	for (guint i=0; i<biff_->size(); i++)
		biff_->mailbox(i)->listed (false);
	GtkTreeIter iter;
	gboolean valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(store), &iter);
	while (valid) {
		guint uin;
		gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, COLUMN_UIN, &uin, -1);
		Mailbox *mailbox = biff_->find (uin);
		if (mailbox) {
			synchronize (mailbox, store, &iter);
			valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
			mailbox->listed (true);
	    }
		else
			valid = gtk_list_store_remove (store, &iter);
	}

	for (guint i=0; i<biff_->size(); i++) {
		Mailbox *mailbox = biff_->mailbox(i);
		if (!mailbox->listed()) {
			gtk_list_store_append (store, &iter);
			synchronize (mailbox, store, &iter);
		}
	}

	// Options
	if (biff_->sound_type_ == SOUND_NONE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("nosound_check")), true);
	else if (biff_->sound_type_ == SOUND_BEEP)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("system_beep_check")), true);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("sound_check")), true);
	gtk_entry_set_text (GTK_ENTRY (get("sound_entry")), biff_->sound_file_.c_str());
	gtk_range_set_value (GTK_RANGE (get ("volume_scale")), biff_->sound_volume_);  
	gtk_entry_set_text (GTK_ENTRY (get("command_entry")), biff_->sound_command_.c_str());
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("poptime_spin")), biff_->popup_time_);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("maxmail_spin")), biff_->max_mail_);
	gtk_entry_set_text (GTK_ENTRY (get("mailapp_entry")), biff_->mail_app_.c_str());
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("display_popup_check")), biff_->popup_display_);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("no_clear_password_check")), biff_->no_clear_password_);

	// Layout: biff window
	GtkImageAnimation *anim;
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("newmail_image")), "_animation_");
	anim->open (biff_->biff_newmail_image_.c_str());
	anim->start();
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("nomail_image")), "_animation_");
	anim->open (biff_->biff_nomail_image_.c_str());
	anim->start();
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("newmail_use_image_check")), biff_->biff_use_newmail_image_);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("newmail_use_text_check")), biff_->biff_use_newmail_text_);
	gtk_entry_set_text (GTK_ENTRY (get("newmail_image_entry")), biff_->biff_newmail_image_.c_str());
	gtk_entry_set_text (GTK_ENTRY (get("newmail_text_entry")), biff_->biff_newmail_text_.c_str());
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("nomail_use_image_check")), biff_->biff_use_nomail_image_);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("nomail_use_text_check")), biff_->biff_use_nomail_text_);
	gtk_entry_set_text (GTK_ENTRY (get("nomail_image_entry")), biff_->biff_nomail_image_.c_str());
	gtk_entry_set_text (GTK_ENTRY (get("nomail_text_entry")), biff_->biff_nomail_text_.c_str());

	// Layout: popup window
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("max_popup_line_spin")), biff_->popup_max_line_);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("max_sender_spin")), biff_->popup_max_sender_size_);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("max_subject_spin")), biff_->popup_max_subject_size_);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("display_date_check")), biff_->popup_display_date_);

	// Geometry: font
	GdkColor color;
	gdk_color_parse (biff_->popup_font_color_.c_str(), &color);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (get("popup_font_color")), &color);
	gdk_color_parse (biff_->biff_font_color_.c_str(), &color);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (get("biff_font_color")), &color);
	gtk_font_button_set_font_name (GTK_FONT_BUTTON(get("biff_font")), biff_->biff_font_.c_str());
	gtk_font_button_set_font_name (GTK_FONT_BUTTON(get("popup_font")), biff_->popup_font_.c_str());	

	// Geometry: popup window
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("popup_use_geometry_check")), biff_->popup_use_geometry_);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("popup_use_decoration_check")), biff_->popup_is_decorated_);
	gtk_entry_set_text (GTK_ENTRY (get("popup_geometry_entry")), biff_->popup_geometry_.c_str());

	// Geometry: biff window
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("biff_use_geometry_check")), biff_->biff_use_geometry_);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("biff_use_decoration_check")), biff_->biff_is_decorated_);
	gtk_entry_set_text (GTK_ENTRY (get("biff_geometry_entry")), biff_->biff_geometry_.c_str());

	// Stop button
	if (biff_->check_mode_ == AUTOMATIC_CHECK)
		biff_->check_mode_ = MANUAL_CHECK;
	else
		biff_->check_mode_ = AUTOMATIC_CHECK;
	on_stop (0);
}


void
Preferences::apply (void)
{
	// Options
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("nosound_check"))))
		biff_->sound_type_ = SOUND_NONE;
	else if	(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("system_beep_check"))))
		biff_->sound_type_ = SOUND_BEEP;
	else
		biff_->sound_type_ = SOUND_FILE;
	biff_->sound_file_ = gtk_entry_get_text (GTK_ENTRY (get("sound_entry")));
	biff_->sound_volume_ = (guint) gtk_range_get_value (GTK_RANGE (get ("volume_scale")));
	biff_->sound_command_ = gtk_entry_get_text (GTK_ENTRY (get("command_entry")));
	biff_->popup_time_ = (guint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(get("poptime_spin")));
	biff_->max_mail_ = (guint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(get("maxmail_spin")));
	biff_->mail_app_ = gtk_entry_get_text (GTK_ENTRY (get("mailapp_entry")));
	biff_->popup_display_ = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("display_popup_check")));
	biff_->no_clear_password_ = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("no_clear_password_check")));

	// Layout: biff window
	biff_->biff_use_newmail_image_ = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("newmail_use_image_check")));
	biff_->biff_use_newmail_text_ = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("newmail_use_text_check")));
	biff_->biff_newmail_image_ = gtk_entry_get_text (GTK_ENTRY (get("newmail_image_entry")));
	biff_->biff_newmail_text_ = gtk_entry_get_text (GTK_ENTRY (get("newmail_text_entry")));
	biff_->biff_use_nomail_image_ = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("nomail_use_image_check")));
	biff_->biff_use_nomail_text_ = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("nomail_use_text_check")));
	biff_->biff_nomail_image_ = gtk_entry_get_text (GTK_ENTRY (get("nomail_image_entry")));
	biff_->biff_nomail_text_ = gtk_entry_get_text (GTK_ENTRY (get("nomail_text_entry")));

	// Layout: popup window
	biff_->popup_max_line_ = (guint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(get("max_popup_line_spin")));
	biff_->popup_max_sender_size_ = (guint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(get("max_sender_spin")));
	biff_->popup_max_subject_size_ = (guint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(get("max_subject_spin")));
	biff_->popup_display_date_ = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("display_date_check")));

	// Geometry: font
	char buffer[16];
	GdkColor color;
	gtk_color_button_get_color (GTK_COLOR_BUTTON (get("popup_font_color")),  &color);
	sprintf (buffer, "#%.2x%.2x%.2x", color.red/256, color.green/256, color.blue/256);
	biff_->popup_font_color_ = buffer;
	gtk_color_button_get_color (GTK_COLOR_BUTTON (get("biff_font_color")),  &color);
	sprintf (buffer, "#%.2x%.2x%.2x", color.red/256, color.green/256, color.blue/256);
	biff_->biff_font_color_ = buffer;
	biff_->popup_font_ = gtk_font_button_get_font_name (GTK_FONT_BUTTON(get("popup_font")));
	biff_->biff_font_ = gtk_font_button_get_font_name (GTK_FONT_BUTTON(get("biff_font")));

	// Geometry: popup window
	biff_->popup_use_geometry_ = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("popup_use_geometry_check")));
	biff_->popup_is_decorated_ = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("popup_use_decoration_check")));
	biff_->popup_geometry_ = gtk_entry_get_text (GTK_ENTRY (get("popup_geometry_entry")));

	// Geometry: biff window
	biff_->biff_use_geometry_ = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("biff_use_geometry_check")));
	biff_->biff_is_decorated_ = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("biff_use_decoration_check")));
	biff_->biff_geometry_ = gtk_entry_get_text (GTK_ENTRY (get("biff_geometry_entry")));
}

void
Preferences::on_add	(GtkWidget *widget)
{
	if (added_ == 0) {
		added_ = new Mailbox(biff_);
		biff_->add (added_);
		synchronize ();
		GtkTreeView  *view  = GTK_TREE_VIEW (get("mailboxes_treeview"));
		GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (view));
		GtkTreeIter iter;
		gboolean valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(store), &iter);
		while (valid) {
			guint uin;
			gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, COLUMN_UIN, &uin, -1);
			if (added_->uin() == uin)
				break;
			valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
		}
		gtk_tree_selection_select_iter  (gtk_tree_view_get_selection (view), &iter);
		properties_->show ();
	}
}

void
Preferences::on_remove (GtkWidget *widget)
{	
	GtkTreeView *view = GTK_TREE_VIEW (get("mailboxes_treeview"));
	GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (view));
		guint uin;
		gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, COLUMN_UIN, &uin, -1);
		biff_->remove (uin);
		properties_->select (0);
		synchronize ();
	}	
}

void
Preferences::on_properties (GtkWidget *widget)
{
	properties_->show();
}

void
Preferences::on_close (GtkWidget *widget)
{
	// Stop animation
	GtkImageAnimation *anim;
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("newmail_image")), "_animation_");
	anim->stop();
	anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("nomail_image")), "_animation_");
	anim->stop();

	// Hide properties
	properties_->hide ();

	// Apply change & save them
	apply ();
	biff_->save ();

	hide();
	biff_->applet()->update();
	biff_->applet()->show();
	if (biff_->check_mode_ == AUTOMATIC_CHECK)
		biff_->applet()->watch_now();
}


void
Preferences::on_stop (GtkWidget *widget)
{
	GtkWidget *child = gtk_bin_get_child (GTK_BIN(get("stop")));
	child  = gtk_bin_get_child (GTK_BIN(child));
	GList *list = gtk_container_get_children (GTK_CONTAINER (child));
	GtkWidget *image = (GtkWidget *) list->data;
	list = list->next;
	GtkWidget *label = (GtkWidget *) list->data;
	if (biff_->check_mode_ == AUTOMATIC_CHECK) {
		gtk_label_set_markup (GTK_LABEL (label), _("_Start"));
		gtk_label_set_use_underline(GTK_LABEL (label), true);
		gtk_image_set_from_stock (GTK_IMAGE (image), GTK_STOCK_EXECUTE, GTK_ICON_SIZE_BUTTON);
		biff_->check_mode_ = MANUAL_CHECK;
	}
	else {
		gtk_label_set_markup (GTK_LABEL (label), _("_Stop"));
		gtk_label_set_use_underline(GTK_LABEL (label), true);
		gtk_image_set_from_stock (GTK_IMAGE (image), GTK_STOCK_STOP, GTK_ICON_SIZE_BUTTON);
		biff_->check_mode_ = AUTOMATIC_CHECK;
	}
}

void
Preferences::on_browse_sound (GtkWidget *widget)
{
	browse (_("Browse for a sound"), "sound_entry");
}

void
Preferences::on_test_sound (GtkWidget *widget)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("system_beep_check"))))
		gdk_beep ();    
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("sound_check")))) {
		gchar *data = (gchar *) gtk_entry_get_text (GTK_ENTRY(get("sound_entry")));
		std::stringstream s;
		s << gfloat (gtk_range_get_value (GTK_RANGE (get ("volume_scale"))))/100.0;
		std::string command = gtk_entry_get_text (GTK_ENTRY(get("command_entry")));
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

void
Preferences::on_browse_mailapp (GtkWidget *widget)
{
	browse (_("Browse for a mail application"), "mailapp_entry");
}

void
Preferences::on_browse_newmail_image (GtkWidget *widget)
{
	GtkWidget *preview = gtk_image_new ();
	gboolean result = browse (_("Browse for a new mail image"), "newmail_image_entry", false, preview);
	if (result == 1) {
		GtkImageAnimation *anim;
		anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("newmail_image")), "_animation_");
		anim->open ((gchar *) gtk_entry_get_text (GTK_ENTRY(get("newmail_image_entry"))));
		anim->start();
	}
}

void
Preferences::on_browse_nomail_image (GtkWidget *widget)
{
	GtkWidget *preview = gtk_image_new ();
	gboolean result = browse (_("Browse for a new mail image"), "nomail_image_entry", false, preview);
	if (result == 1) {
		GtkImageAnimation *anim;
		anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("nomail_image")), "_animation_");
		anim->open ((gchar *) gtk_entry_get_text (GTK_ENTRY(get("nomail_image_entry"))));
		anim->start();		
	}
}

gboolean
Preferences::on_destroy (GtkWidget *widget,  GdkEvent *event)
{
	hide ();
	return TRUE;
}

gboolean
Preferences::on_delete (GtkWidget *widget,  GdkEvent *event)
{
	hide ();
	return true;
}

void
Preferences::on_selection (GtkTreeSelection *selection)
{
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		GtkTreeView  *view  = GTK_TREE_VIEW (get("mailboxes_treeview"));
		GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (view));
		guint uin;
		gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, COLUMN_UIN, &uin, -1);
		Mailbox *mailbox = biff_->find (uin);
		properties_->select (mailbox);
		gtk_label_set_text (GTK_LABEL(get ("selection")), mailbox->name().c_str());
	}
	else {
		gtk_label_set_text (GTK_LABEL(get ("selection")), _("No mailbox selected"));
	}
}
