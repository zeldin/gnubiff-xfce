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

#include "support.h"

#include <sstream>

#include "ui-preferences.h"
#include "ui-properties.h"
#include "ui-applet.h"
#include "gtk_image_animation.h"
#include "biff.h"
#include "mailbox.h"


/* "C" bindings */
extern "C" {

	gboolean PREFERENCES_on_click (GtkWidget *widget,
								   GdkEventButton *event,
								   gpointer data)
	{
		if ((event->button == 1) && (event->type == GDK_2BUTTON_PRESS))
			PREFERENCES(data)->on_properties (0);
		return FALSE;
	}

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

	void PREFERENCES_on_check_changed (GtkWidget *widget,
								  gpointer data)
	{
		PREFERENCES(data)->on_check_changed (widget);
	}
}



Preferences::Preferences (Biff *biff) : GUI (GNUBIFF_DATADIR"/preferences.glade")
{
	biff_ = biff;
	properties_ = new Properties (this);
	properties_->create ();
	selected_ = 0;
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
											  G_TYPE_STRING);
	GtkTreeView *view = GTK_TREE_VIEW (get("mailboxes_treeview"));
	gtk_tree_view_set_model (view, GTK_TREE_MODEL(store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (view), TRUE);

	GtkTreeViewColumn *column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Mailbox"));
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_MAILBOX);

	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "stock-id", COLUMN_MAILBOX_STOCK_ID);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text", COLUMN_MAILBOX);
	gtk_tree_view_column_set_expand (column, true);
	gtk_tree_view_append_column (view, column);

	column = gtk_tree_view_column_new_with_attributes (_("Type"),
													   gtk_cell_renderer_text_new(),
													   "text", COLUMN_FORMAT,
													   NULL);
	gtk_tree_view_column_set_resizable(column, FALSE);
	gtk_tree_view_column_set_sort_column_id(column, COLUMN_FORMAT);
	gtk_tree_view_append_column (view, column);

	column = gtk_tree_view_column_new_with_attributes ("",
													   gtk_cell_renderer_pixbuf_new (),
													   "stock-id", COLUMN_STATUS_STOCK_ID,
													   NULL);
	GtkWidget *image = gtk_image_new_from_stock ("gtk-execute", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image);
	gtk_tree_view_column_set_widget (GTK_TREE_VIEW_COLUMN (column), image);
	gtk_tree_view_column_set_resizable(column, FALSE);
	gtk_tree_view_column_set_sort_column_id(column, COLUMN_STATUS_STOCK_ID);
	gtk_tree_view_append_column (view, column);
	GtkTooltips *tooltips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (tooltips, image->parent->parent->parent, _("Status"), "");
	gtk_tooltips_enable (tooltips);

	column = gtk_tree_view_column_new_with_attributes ("",
													   gtk_cell_renderer_pixbuf_new (),
													   "stock-id", COLUMN_SECURITY_STOCK_ID,
													   NULL);
	image = gtk_image_new_from_stock ("gtk-dialog-authentication", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image);
	gtk_tree_view_column_set_widget (GTK_TREE_VIEW_COLUMN (column), image);
	gtk_tree_view_column_set_resizable (column, FALSE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_SECURITY_STOCK_ID);
	gtk_tree_view_append_column (view, column);

	tooltips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (tooltips, image->parent->parent->parent, _("Security"), "");
	gtk_tooltips_enable (tooltips);


	gtk_tree_view_set_search_column (view, COLUMN_MAILBOX);
  
	GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT(selection), "changed",
					  G_CALLBACK(PREFERENCES_on_selection_changed), this);

	// Selection label
	gtk_label_set_text (GTK_LABEL(get ("selection")), _("No mailbox selected"));

	if (biff_->ui_mode_ == GNOME_MODE) {
		gtk_widget_set_sensitive (get("applet_geometry_check"), false);
		gtk_widget_set_sensitive (get("applet_geometry_entry"), false);
		gtk_widget_set_sensitive (get("applet_decoration_check"), false);
	}

	return true;
}

void
Preferences::show (std::string name)
{
	if (!xml_)
		return;
	synchronize ();
	biff_->applet()->stop ();
	gtk_widget_show (get(name));
}

void Preferences:: hide (std::string name) {
	if (!xml_)
		return;
	gtk_widget_hide (get(name));
}


void
Preferences::synchronize (class Mailbox *mailbox, GtkListStore *store, GtkTreeIter *iter)
{
	if (mailbox) {
		std::string stock_mailbox = "gtk-network";
		std::string stock_status  = "gtk-dialog-error";

		if ((mailbox->protocol() == PROTOCOL_FILE) ||
			(mailbox->protocol() == PROTOCOL_MH) ||
			(mailbox->protocol() == PROTOCOL_MAILDIR))
			stock_mailbox = "gtk-home";
		else if (mailbox->protocol() == PROTOCOL_NONE)
			stock_mailbox ="gtk-dialog-question";

		if (mailbox->status() == MAILBOX_UNKNOWN)
			stock_status = "gtk-dialog-question";
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
							COLUMN_MAILBOX_STOCK_ID, stock_mailbox.c_str(),
							COLUMN_MAILBOX, mailbox->name().c_str(),
							COLUMN_SECURITY_STOCK_ID, "gtk-ok",
							COLUMN_FORMAT, format.c_str(),
							COLUMN_STATUS_STOCK_ID, stock_status.c_str(),
							-1);

		if (mailbox->protocol() == PROTOCOL_NONE)
			gtk_list_store_set (store, iter, COLUMN_SECURITY_STOCK_ID, "gtk-dialog-question", -1);
		else if (mailbox->authentication() == (gint) AUTH_USER_PASS)
			gtk_list_store_set (store, iter, COLUMN_SECURITY_STOCK_ID, "gtk-no", -1);
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
		Mailbox *mailbox = biff_->get (uin);
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

	// Mailboxes page
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("max_mail_check")), 			biff_->use_max_mail_);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("max_mail_spin")),					biff_->max_mail_);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("newmail_command_check")),		biff_->use_newmail_command_);
	gtk_entry_set_text (GTK_ENTRY (get("newmail_command_entry")),						biff_->newmail_command_.c_str());
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("double_command_check")),		biff_->use_double_command_);
	gtk_entry_set_text (GTK_ENTRY (get("double_command_entry")),						biff_->double_command_.c_str());
	
	// Applet page
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("applet_geometry_check")), 		biff_->applet_use_geometry_);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("applet_decoration_check")),	biff_->applet_use_decoration_);
	gtk_entry_set_text (GTK_ENTRY (get("applet_geometry_entry")),						biff_->applet_geometry_.c_str());
	gtk_font_button_set_font_name (GTK_FONT_BUTTON(get("applet_font_button")),			biff_->applet_font_.c_str());

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("newmail_text_check")),			biff_->use_newmail_text_);
	gtk_entry_set_text (GTK_ENTRY (get("newmail_text_entry")),							biff_->newmail_text_.c_str());
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("newmail_image_check")),		biff_->use_newmail_image_);
	gtk_entry_set_text (GTK_ENTRY (get("newmail_image_entry")),							biff_->newmail_image_.c_str());

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("nomail_text_check")),			biff_->use_nomail_text_);
	gtk_entry_set_text (GTK_ENTRY (get("nomail_text_entry")),							biff_->nomail_text_.c_str());
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("nomail_image_check")),			biff_->use_nomail_image_);
	gtk_entry_set_text (GTK_ENTRY (get("nomail_image_entry")),							biff_->nomail_image_.c_str());


	// Popup page
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("use_popup_check")),			biff_->use_popup_);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("popup_delay_spin")),				biff_->popup_delay_);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("popup_geometry_check")),		biff_->popup_use_geometry_);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("popup_decoration_check")),		biff_->popup_use_decoration_);
	gtk_entry_set_text (GTK_ENTRY (get("popup_geometry_entry")),						biff_->popup_geometry_.c_str());
	gtk_font_button_set_font_name (GTK_FONT_BUTTON(get("popup_font_button")),			biff_->popup_font_.c_str());

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("popup_size_check")),			biff_->popup_use_size_);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("popup_size_spin")),					biff_->popup_size_);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("popup_format_check")),			biff_->popup_use_format_);
	gtk_entry_set_text (GTK_ENTRY (get("popup_format_entry")),							biff_->popup_format_.c_str());


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
	// Mailboxes page
	biff_->use_max_mail_        = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("max_mail_check")));
	biff_->max_mail_            = (guint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(get("max_mail_spin")));
	biff_->use_newmail_command_ = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("newmail_command_check")));
	biff_->newmail_command_     = gtk_entry_get_text (GTK_ENTRY (get("newmail_command_entry")));
	biff_->use_double_command_  = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("double_command_check")));
	biff_->double_command_      = gtk_entry_get_text (GTK_ENTRY (get("double_command_entry")));

	// Applet page
	biff_->applet_use_geometry_   = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("applet_geometry_check")));
	biff_->applet_use_decoration_ =	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("applet_decoration_check")));
	biff_->applet_geometry_       =	gtk_entry_get_text (GTK_ENTRY (get("applet_geometry_entry")));
	biff_->applet_font_           = gtk_font_button_get_font_name (GTK_FONT_BUTTON(get("applet_font_button")));

	biff_->use_newmail_text_      = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("newmail_text_check")));
	biff_->newmail_text_          = gtk_entry_get_text (GTK_ENTRY (get("newmail_text_entry")));
	biff_->use_newmail_image_     = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("newmail_image_check")));
	biff_->newmail_image_         = gtk_entry_get_text (GTK_ENTRY (get("newmail_image_entry")));

	biff_->use_nomail_text_       = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("nomail_text_check")));
	biff_->nomail_text_           = gtk_entry_get_text (GTK_ENTRY (get("nomail_text_entry")));
	biff_->use_nomail_image_      = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("nomail_image_check")));
	biff_->nomail_image_          = gtk_entry_get_text (GTK_ENTRY (get("nomail_image_entry")));

	// Popup page
	biff_->use_popup_             = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("use_popup_check")));
	biff_->popup_delay_           = (guint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(get("popup_delay_spin")));

	biff_->popup_use_geometry_    = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("popup_geometry_check")));
	biff_->popup_use_decoration_  = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("popup_decoration_check")));
	biff_->popup_geometry_        = gtk_entry_get_text (GTK_ENTRY (get("popup_geometry_entry")));
	biff_->popup_font_            = gtk_font_button_get_font_name (GTK_FONT_BUTTON(get("popup_font_button")));

	biff_->popup_use_size_        = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("popup_size_check")));
	biff_->popup_size_            = (guint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(get("popup_size_spin")));

	biff_->popup_use_format_      = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("popup_format_check")));
	biff_->popup_format (gtk_entry_get_text (GTK_ENTRY (get("popup_format_entry"))));
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
		biff_->remove (biff_->get(uin));
		properties_->select (0);
		synchronize ();
	}	
}

void
Preferences::on_properties (GtkWidget *widget)
{
	if (selected_)
		properties_->show();
}

void
Preferences::on_close (GtkWidget *widget)
{

	// Hide properties
	properties_->hide ();

	// Apply change & save them
	apply ();
	biff_->save ();
	hide();
	if (biff_->check_mode_ == AUTOMATIC_CHECK)
		biff_->applet()->start (3);
	biff_->applet()->update();
	biff_->applet()->show();
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
Preferences::on_browse_newmail_image (GtkWidget *widget)
{
	GtkWidget *preview = gtk_image_new ();
	browse (_("Browse for a new mail image"), "newmail_image_entry", false, preview);
}

void
Preferences::on_browse_nomail_image (GtkWidget *widget)
{
	GtkWidget *preview = gtk_image_new ();
	browse (_("Browse for a new mail image"), "nomail_image_entry", false, preview);
}

gboolean
Preferences::on_destroy (GtkWidget *widget,  GdkEvent *event)
{
	hide ();
	if (biff_->check_mode_ == AUTOMATIC_CHECK)
		biff_->applet()->start (3);
	biff_->applet()->update();
	biff_->applet()->show();
	return true;
}

gboolean
Preferences::on_delete (GtkWidget *widget,  GdkEvent *event)
{
	hide ();
	if (biff_->check_mode_ == AUTOMATIC_CHECK)
		biff_->applet()->start (3);
	biff_->applet()->update();
	biff_->applet()->show();
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
		Mailbox *mailbox = biff_->get (uin);
		properties_->select (mailbox);
		selected_ = mailbox;
		gtk_label_set_text (GTK_LABEL(get ("selection")), mailbox->name().c_str());
	}
	else {
		gtk_label_set_text (GTK_LABEL(get ("selection")), _("No mailbox selected"));
		selected_ = 0;
	}
}

void
Preferences::on_check_changed (GtkWidget *widget)
{
	gtk_widget_set_sensitive (get("max_mail_spin"),
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("max_mail_check"))));
	gtk_widget_set_sensitive (get("newmail_command_entry"),
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("newmail_command_check"))));
	gtk_widget_set_sensitive (get("double_command_entry"),
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("double_command_check"))));
	gtk_widget_set_sensitive (get("applet_geometry_entry"),
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("applet_geometry_check"))));
	gtk_widget_set_sensitive (get("newmail_text_entry"),
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("newmail_text_check"))));
	gtk_widget_set_sensitive (get("newmail_image_entry"),
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("newmail_image_check"))));
	gtk_widget_set_sensitive (get("newmail_image_browse"),
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("newmail_image_check"))));
	gtk_widget_set_sensitive (get("nomail_text_entry"),
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("nomail_text_check"))));
	gtk_widget_set_sensitive (get("nomail_image_entry"), 
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("nomail_image_check"))));
	gtk_widget_set_sensitive (get("nomail_image_browse"),
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("nomail_image_check"))));
	gtk_widget_set_sensitive (get("popup_delay_spin"),
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("use_popup_check"))));
	gtk_widget_set_sensitive (get("popup_geometry_entry"),
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("popup_geometry_check"))));
	gtk_widget_set_sensitive (get("popup_size_spin"),
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("popup_size_check"))));
	gtk_widget_set_sensitive (get("popup_format_entry"),
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("popup_format_check"))));
}
