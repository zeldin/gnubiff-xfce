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

	void PREFERENCES_on_Notebook_switch_page (GtkNotebook *widget,
											  GtkNotebookPage *page,
											  gint page_num, gpointer data)
	{
		// FIXME: Do not hardcode page 3
		if (page_num == 3)
			PREFERENCES(data)->expert_add_option_list ();
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
	GUI::create ();
	expert_create ();

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

	return true;
}

/**
 *  Create the expert option editing dialog.
 */
void 
Preferences::expert_create (void)
{
	if (!biff_->value_bool ("use_expert"))
		return;

	GtkListStore *store = gtk_list_store_new (COL_EXP_N, G_TYPE_INT,
											  G_TYPE_STRING, G_TYPE_STRING,
											  G_TYPE_STRING, G_TYPE_STRING);
	GtkTreeView *view = GTK_TREE_VIEW (get("expert_treeview"));
	gtk_tree_view_set_model (view, GTK_TREE_MODEL(store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (view), TRUE);

	GtkTreeViewColumn *column;

	// Column: NAME
	column = gtk_tree_view_column_new_with_attributes ("Option", gtk_cell_renderer_text_new(), "text", COL_EXP_GROUPNAME, NULL);
	gtk_tree_view_column_set_resizable(column, false);
	gtk_tree_view_column_set_sort_column_id(column, COL_EXP_GROUPNAME);
	gtk_tree_view_append_column (view, column);

	// Column: TYPE
	column = gtk_tree_view_column_new_with_attributes ("Type", gtk_cell_renderer_text_new(), "text", COL_EXP_TYPE, NULL);
	gtk_tree_view_column_set_resizable(column, false);
	gtk_tree_view_column_set_sort_column_id(column, COL_EXP_TYPE);
	gtk_tree_view_append_column (view, column);

	// Column: VALUE
	column = gtk_tree_view_column_new_with_attributes ("Value", gtk_cell_renderer_text_new(), "text", COL_EXP_VALUE, NULL);
	gtk_tree_view_column_set_resizable(column, false);
	gtk_tree_view_column_set_sort_column_id(column, COL_EXP_VALUE);
	gtk_tree_view_append_column (view, column);

	gtk_tree_view_set_search_column (view, COL_EXP_GROUPNAME);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
}

/**
 *  Update list of options. Put all options and their values into the list.
 *
 *  @param widget Expert dialog vbox widget
 */
void
Preferences::expert_add_option_list (void)
{
	if (!biff_->value_bool ("use_expert"))
		return;

	GtkTreeView  *view  = GTK_TREE_VIEW (get("expert_treeview"));
	GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (view));
	GtkTreeIter iter;

	// Clear old options
	gtk_list_store_clear (store);

	std::map<std::string, Option *> opts;
	std::map<std::string, Option *>::iterator it;

	// Add options
	for (int i = -1; i < (signed)biff_->size(); i++) {
		if (i == -1)
			biff_->options (opts);
		else
			biff_->mailbox(i)->options (opts);
		it = opts.begin();
		while (it != opts.end()) {
			// Create displayed name by concatenating group and name
			std::string groupname;
			if (i == -1) {
				groupname  = biff_->group_string (it->second->group());
				groupname += "/" + it->first;
			}
			else {
				std::stringstream ss;
				ss << "mailbox[" << i << "]/" << it->first;
				ss >> groupname;
			}
			// Store values
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
								COL_EXP_ID, i,
								COL_EXP_NAME, it->first.c_str(),
								COL_EXP_GROUPNAME, groupname.c_str(),
								COL_EXP_TYPE,it->second->type_string().c_str(),
								COL_EXP_VALUE, it->second->to_string().c_str(),
								-1);
			it++;
		}
	}
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

		std::string format = mailbox->value_to_string ("protocol",
													   mailbox->protocol());

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

	// Insert the values of the options into the GUI elements
	biff_->gui_set (OPTGRP_ALL, xml_, filename_);

	// Stop button
	if (biff_->value_uint ("check_mode") == AUTOMATIC_CHECK)
		biff_->value ("check_mode", MANUAL_CHECK);
	else
		biff_->value ("check_mode", AUTOMATIC_CHECK);
	on_stop (0);
}


void
Preferences::apply (void)
{
	// Retrieve all values of the options from the GUI elements
	biff_->gui_get (OPTGRP_ALL, xml_, filename_);
}

void
Preferences::on_add	(GtkWidget *widget)
{
	if (added_ == 0) {
		added_ = new Mailbox(biff_);
		if (selected_)
			(*added_) = (*selected_);

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
	if (biff_->value_uint ("check_mode") == AUTOMATIC_CHECK)
		biff_->applet()->start (3);
	biff_->applet()->update(true);
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
	if (biff_->value_uint ("check_mode") == AUTOMATIC_CHECK) {
		gtk_label_set_markup (GTK_LABEL (label), _("_Start"));
		gtk_label_set_use_underline(GTK_LABEL (label), true);
		gtk_image_set_from_stock (GTK_IMAGE (image), GTK_STOCK_EXECUTE, GTK_ICON_SIZE_BUTTON);
		biff_->value ("check_mode", MANUAL_CHECK);
	}
	else {
		gtk_label_set_markup (GTK_LABEL (label), _("_Stop"));
		gtk_label_set_use_underline(GTK_LABEL (label), true);
		gtk_image_set_from_stock (GTK_IMAGE (image), GTK_STOCK_STOP, GTK_ICON_SIZE_BUTTON);
		biff_->value ("check_mode", AUTOMATIC_CHECK);
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
	if (biff_->value_uint ("check_mode") == AUTOMATIC_CHECK)
		biff_->applet()->start (3);
	biff_->applet()->update(true);
	biff_->applet()->show();
	return true;
}

gboolean
Preferences::on_delete (GtkWidget *widget,  GdkEvent *event)
{
	hide ();
	if (biff_->value_uint ("check_mode") == AUTOMATIC_CHECK)
		biff_->applet()->start (3);
	biff_->applet()->update(true);
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
		gtk_button_set_label (GTK_BUTTON(get("add")), "gtk-copy");
	}
	else {
		gtk_label_set_text (GTK_LABEL(get ("selection")), _("No mailbox selected"));
		gtk_button_set_label (GTK_BUTTON(get("add")), "gtk-add");
		selected_ = 0;
	}
}

void
Preferences::on_check_changed (GtkWidget *widget)
{
	// Disable and enable certain GUI elements depending on values of some
	// options
	biff_->gui_show (OPTGRP_ALL, xml_, filename_);
}
