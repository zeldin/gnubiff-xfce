// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2007 Nicolas Rougier, 2004-2007 Robert Sowada
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
#include <math.h>

#include "ui-preferences.h"
#include "ui-applet.h"
#include "ui-popup.h"
#include "mailbox.h"


/**
 * "C" binding
 **/

extern "C" {
	gboolean POPUP_on_popdown (gpointer data)
	{
		if (data)
			return ((Popup *) data)->on_popdown ();
		else
			unknown_internal_error ();
		return false;
	}

	gboolean POPUP_on_button_press (GtkWidget *widget,
									GdkEventButton *event,
									gpointer data)
	{
		if (data)
			return ((Popup *) data)->on_button_press (event);
		else
			unknown_internal_error ();
		return false;
	}

	gboolean POPUP_on_button_release (GtkWidget *widget,
									  GdkEventButton *event,
									  gpointer data)
	{
		if (data)
			return ((Popup *) data)->on_button_release (event);
		else
			unknown_internal_error ();
		return false;
	}

	void POPUP_on_enter (GtkWidget *widget,
						 GdkEventCrossing *event,
						 gpointer data)
	{
		if (data)
			((Popup *) data)->on_enter (event);
		else
			unknown_internal_error ();
	}

	void POPUP_on_leave (GtkWidget *widget,
						 GdkEventCrossing *event,
						 gpointer data)
	{
		if (data)
			((Popup *) data)->on_leave (event);
		else
			unknown_internal_error ();
	}
	
	void POPUP_on_select (GtkTreeSelection *selection,
						  gpointer data)
	{
		if (data)
			((Popup *) data)->on_select (selection);
		else
			unknown_internal_error ();
	}
}

Popup::Popup (Biff *biff) : GUI (GNUBIFF_DATADIR"/popup.glade")
{
	biff_ = biff;
	poptag_ = 0;
	tree_selection_ = 0;
	consulting_ = false;

	// Create mutexes
	timer_mutex_ = g_mutex_new ();
}

Popup::~Popup (void)
{
	// Free all mutexes
	g_mutex_lock (timer_mutex_);
	g_mutex_unlock (timer_mutex_);
	g_mutex_free (timer_mutex_);

	// Free allocated memory
	free_stored_strings ();
}

/**
 *  Free all strings that have been stored for displaying the popup.
 */
void 
Popup::free_stored_strings (void)
{
	GtkListStore *store;

	// Get tree store and clear it
	store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (get ("treeview"))));
	if (store)
		gtk_list_store_clear (store);

	// Free memory of strings displayed in popup previously
	for (guint i=0; i<stored_strings_.size(); i++)
		g_free (stored_strings_[i]);
	stored_strings_.clear();
}

gint
Popup::create (gpointer callbackdata)
{
	GUI::create(this);

	GtkTreeModel *model;

	GtkListStore *store;
	store = gtk_list_store_new (COLUMNS,
								G_TYPE_STRING,  // Mailbox name
								G_TYPE_STRING,	// Number
								G_TYPE_STRING,	// Sender
								G_TYPE_STRING,	// Subject
								G_TYPE_STRING,	// Date
								G_TYPE_POINTER); // Pointer to the array element
	model = GTK_TREE_MODEL (store);

	GtkWidget *treeview = get("treeview");
	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), model);
	gtk_widget_set_events (treeview,
						   GDK_ENTER_NOTIFY_MASK |
						   GDK_LEAVE_NOTIFY_MASK |
						   GDK_BUTTON_PRESS_MASK);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview), 0);
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW (treeview));


	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Mailbox"), renderer,
													   "text", COLUMN_NAME,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("#", renderer,
													   "text", COLUMN_NUMBER,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("From"), renderer,
													   "text", COLUMN_SENDER,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Subject"), renderer,
													   "text", COLUMN_SUBJECT,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Date"), renderer,
													   "text", COLUMN_DATE,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  
	// Callback on selection
	GtkTreeSelection *select = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (select), "changed", G_CALLBACK (POPUP_on_select), this);  

	g_object_unref (G_OBJECT (model));

	// Remove window decoration from mail body popup
	gtk_window_set_decorated (GTK_WINDOW(get("popup")), FALSE);

	// Some text tag used for displaying mail content
	GtkWidget *view = get("textview");
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	gtk_text_buffer_create_tag (buffer, "bold",
								"weight", PANGO_WEIGHT_BOLD,
								"size", 9 * PANGO_SCALE,
								NULL);
	gtk_text_buffer_create_tag (buffer, "blue",
								"foreground", "blue",
								"size", 9 * PANGO_SCALE,
								NULL);
	gtk_text_buffer_create_tag (buffer, "normal",
								"size", 9 * PANGO_SCALE,
								NULL);

	// Black frame for the mail content header
	GdkColor color;
	gdk_color_parse ("Black", &color);
	gtk_widget_modify_bg (get("ebox_out"), GTK_STATE_NORMAL, &color);
	gtk_widget_set_state (get("ebox_in"), GTK_STATE_PRELIGHT);

	// That's it
	return true;
}


/** 
 * Update popup's list of messages and create the widgets necassary
 * for showing the popup.
 *
 * @return number of headers shown in the popup
 **/
guint 
Popup::update (void)
{
	GtkListStore *store;
	GtkTreeIter iter;
  
	// Free stored strings and gtk list store
	free_stored_strings ();

	// Get list store
	store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (get ("treeview"))));

	// Get headers to be displayed in the popup
	std::vector<Header *> headers;
	headers = biff_->get_message_headers (biff_->value_bool ("popup_use_size"),
										  biff_->value_uint ("popup_size"));

	// Sort headers
	gboolean mb;
	mb = Header::sort_headers (headers, biff_->value_string ("popup_sort_by"));

	// Now we populate the list
	std::vector<Header *>::iterator h = headers.begin();
	std::vector<Header *>::iterator he = headers.end();
	std::set<guint> firstmb;
	while (h != he) {
		gtk_list_store_append (store, &iter);

		guint size = 255;
		if (biff_->value_bool ("popup_use_format"))
			size = 1;

		// Subject
		gchar *subject = utf8_strndup ((*h)->subject().c_str(), std::max<guint> (size, biff_->value_uint ("popup_size_subject")));

		// Date
		gchar *date = utf8_strndup ((*h)->date().c_str(), std::max<guint> (size, biff_->value_uint ("popup_size_date")));

		// Sender
		gchar *sender = utf8_strndup ((*h)->sender().c_str(), std::max<guint> (size, biff_->value_uint ("popup_size_sender")));

		// Mail identifier
		gchar *mailid = g_strdup ((*h)->mailid().c_str());

		// The strings have to be stored as long as the popup might be shown
		stored_strings_.push_back (subject);
		stored_strings_.push_back (date);
		stored_strings_.push_back (sender);
		stored_strings_.push_back (mailid);

		std::stringstream s;
		s << (*h)->position();

		if ((!mb) || (firstmb.find ((*h)->mailbox_uin()) == firstmb.end())) {
			// Mark mailbox as visited
			firstmb.insert ((*h)->mailbox_uin());
			gtk_list_store_set (store, &iter, COLUMN_NAME, biff_->get((*h)->mailbox_uin())->name().c_str(), -1);
		}
		else
			gtk_list_store_set (store, &iter, COLUMN_NAME, "", -1);
		gtk_list_store_set (store, &iter,
							COLUMN_NUMBER, s.str().c_str(),
							COLUMN_SENDER, sender, 
							COLUMN_SUBJECT, subject,
							COLUMN_DATE, date,
							COLUMN_MAILID, mailid,
							-1);
		// Free header and advance to next header
		delete (*h);
		h++;
	}

	// Update window decoration
	gtk_window_set_decorated (GTK_WINDOW(get("dialog")),
							  biff_->value_bool ("popup_use_decoration"));

	// Update fonts
	GtkWidget *treeview = get("treeview");
	PangoFontDescription *font;
	font = pango_font_description_from_string (biff_->value_gchar ("popup_font"));
	gtk_widget_modify_font (treeview, font);
	pango_font_description_free (font);

	if (biff_->value_bool ("popup_use_format")) {
		if (biff_->value_uint ("popup_size_subject") == 0) {
			GtkTreeViewColumn *column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), COLUMN_SUBJECT);
			gtk_tree_view_column_set_visible (column, false);
		}
		if (biff_->value_uint ("popup_size_sender") == 0) {
			GtkTreeViewColumn *column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), COLUMN_SENDER);
			gtk_tree_view_column_set_visible (column, false);
		}
		if (biff_->value_uint ("popup_size_date") == 0) {
			GtkTreeViewColumn *column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), COLUMN_DATE);
			gtk_tree_view_column_set_visible (column, false);
		}
	}

	return headers.size();
}

/**
 *  Hide the popup dialog.
 *
 *  @param  name  Widget's name of the dialog. The default is "dialog".
 */
void 
Popup::hide (std::string name)
{
	g_mutex_lock (timer_mutex_);
	if (poptag_ > 0)
		g_source_remove (poptag_);
	poptag_ = 0;
	g_mutex_unlock (timer_mutex_);

	GUI::hide ();
	gtk_widget_hide (get("popup"));

	consulting_ = false;
}

void
Popup::show (std::string name)
{
	tree_selection_ = 0;
	consulting_ = false;

	GtkWindow *dialog = GTK_WINDOW (get ("dialog"));

	// Present the popup window. Keyboard focus should not be obtained
	// automatically when presenting the window but may then gained manually
	gtk_window_deiconify (dialog);
	gtk_window_set_accept_focus (dialog,
								 biff_->value_bool ("popup_accept_focus"));
	gtk_window_set_focus_on_map (dialog, false);
	gtk_widget_show (GTK_WIDGET (dialog));
//	gtk_window_present (dialog);

	
	if (biff_->value_bool ("popup_use_geometry"))
		gtk_window_parse_geometry (dialog,
								   biff_->value_gchar ("popup_geometry"));
	if (biff_->value_bool ("popup_be_sticky"))
		gtk_window_stick (dialog);
	else
		gtk_window_unstick (dialog);
	gtk_window_set_keep_above (dialog, biff_->value_bool ("popup_keep_above"));
	gtk_window_set_skip_pager_hint (dialog,
									!biff_->value_bool ("popup_pager"));

	g_mutex_lock (timer_mutex_);
	if (poptag_ > 0) 
		g_source_remove (poptag_);
	poptag_ = g_timeout_add (biff_->value_uint ("popup_delay")*1000,
							 POPUP_on_popdown, this);
	g_mutex_unlock (timer_mutex_);

	if (tree_selection_)
		gtk_tree_selection_unselect_all (tree_selection_);
}

gboolean
Popup::on_delete (GtkWidget *widget, GdkEvent *event)
{
	hide ();
	return true;
}

gboolean
Popup::on_popdown (void)
{
	hide();
	return false;
}

gboolean
Popup::on_button_press (GdkEventButton *event)
{
	if (event->button == 1) {
		// This flag is set to warn "on_select" that we would like to
		// consult mail content. We cannot do that here because this
		// button press event will be called before the new selection is
		// made
		consulting_ = true;
		gint root_x, root_y;
		gtk_window_get_position (GTK_WINDOW (get("dialog")), &root_x, &root_y);
		x_ = gint (event->x) + root_x;
		y_ = gint (event->y) + root_y;

	}	
	else if (event->button == 2) {
	}
	else if (event->button == 3)
		hide ();
	return false;
}

gboolean
Popup::on_button_release (GdkEventButton *event)
{
	if (event->button == 1) {
		gtk_widget_hide (get("popup"));
		consulting_ = false;
		if (tree_selection_)
			gtk_tree_selection_unselect_all (tree_selection_);
	}
	return false;
}

void
Popup::on_enter (GdkEventCrossing *event)
{
	g_mutex_lock (timer_mutex_);
	if (poptag_ > 0)
		g_source_remove (poptag_);
	poptag_ = 0;
	g_mutex_unlock (timer_mutex_);
}


void
Popup::on_leave (GdkEventCrossing *event)
{
	if (!consulting_) {
		g_mutex_lock (timer_mutex_);
		if (poptag_ > 0)
			g_source_remove (poptag_);  
		poptag_ = g_timeout_add (biff_->value_uint ("popup_delay")*1000,
								 POPUP_on_popdown, this);
		g_mutex_unlock (timer_mutex_);
	}
}

void
Popup::on_select (GtkTreeSelection *selection)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	tree_selection_ = selection;
	gchar *text;

	// We get the adress of the selected header by getting field 6 of
	// the store model where we stored this info
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gpointer *address;
		gtk_tree_model_get (model, &iter, COLUMN_MAILID, &address, -1);
		if (!biff_->find_message (std::string((gchar *)address),
								  selected_header_))
			return;
	}

	// If we're in consulting mode, update the text of the (single) mail popup
	if (consulting_) {
		// Nop popdown when we're consulting an email
		g_mutex_lock (timer_mutex_);
		if (poptag_ > 0)
			g_source_remove (poptag_);
		poptag_ = 0;
		g_mutex_unlock (timer_mutex_);

		// Show popup window for mail displaying 
		// Name is stupid since we're in Popup 
		//  => "dialog" is the name of the real popup
		//  => "popup" is the name of the mail content window
		gtk_widget_show_all (get("popup"));
		gtk_window_move (GTK_WINDOW(get("popup")), x_, y_); 

		GtkWidget *view = get("textview");
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(view));
		GtkTextIter iter;
		gtk_text_buffer_set_text (buffer, "", -1);
		gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);

		gchar *markup = NULL;

		// Sender
		markup = g_markup_printf_escaped ("<small>%s</small>",
										  selected_header_.sender().c_str());
		if (markup) {
			gtk_label_set_markup (GTK_LABEL(get("from")), markup);
			g_free (markup);
		}

		// Subject
		markup = g_markup_printf_escaped ("<small>%s</small>",
										  selected_header_.subject().c_str());
		if (markup) {
			gtk_label_set_markup (GTK_LABEL(get("subject")), markup);
			g_free (markup);
		}

		// Date
		markup = g_markup_printf_escaped ("<small>%s</small>",
										  selected_header_.date().c_str());
		if (markup) {
			gtk_label_set_markup (GTK_LABEL(get("date")), markup);
			g_free (markup);
		}

		// Body
		text = charset_to_utf8 (selected_header_.body(),
								selected_header_.charset(), biff_);
		if (text) {
			gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, text, -1,
													  "normal", NULL);
			g_free (text);
		}
	}
}
