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
#include <cstdio>
#include <string>
#include <glib.h>
#include <math.h>
#include "ui-popup.h"
#include "mailbox.h"
#include "nls.h"


/**
 * "C" binding
 **/

extern "C" {
	gboolean POPUP_on_popdown (gpointer data)
	{
		return ((Popup *) data)->on_popdown ();
	}

	gboolean POPUP_on_button_press (GtkWidget *widget,
									GdkEventButton *event,
									gpointer data)
	{
		return ((Popup *) data)->on_button_press (event);
	}

	gboolean POPUP_on_button_release (GtkWidget *widget,
									  GdkEventButton *event,
									  gpointer data)
	{
		return ((Popup *) data)->on_button_release (event);
	}

	void POPUP_on_enter (GtkWidget *widget,
						 GdkEventCrossing *event,
						 gpointer data)
	{
		((Popup *) data)->on_enter (event);
	}

	void POPUP_on_leave (GtkWidget *widget,
						 GdkEventCrossing *event,
						 gpointer data)
	{
		((Popup *) data)->on_leave (event);
	}
	
	void POPUP_on_select (GtkTreeSelection *selection,
						  gpointer data)
	{
		((Popup *) data)->on_select (selection);
	}
}

GStaticMutex Popup::timer_mutex_ = G_STATIC_MUTEX_INIT;

Popup::Popup (Biff *biff) : GUI (GNUBIFF_DATADIR"/popup.glade")
{
	biff_ = biff;
	g_static_mutex_lock (&timer_mutex_);
	poptag_ = 0;
	g_static_mutex_unlock (&timer_mutex_);
	tree_selection_ = 0;
	selected_header_ = 0;
	consulting_ = false;
}

Popup::~Popup (void)
{
}

gint
Popup::create (void)
{
	GUI::create();

	GtkTreeModel *model;

	GtkListStore *store;
	store = gtk_list_store_new (COLUMNS,
								G_TYPE_STRING,  // Mailbox name
								G_TYPE_STRING,	// Number
								G_TYPE_STRING,	// Sender
								G_TYPE_STRING,	// Subject
								G_TYPE_STRING,	// Date
								G_TYPE_STRING,	// Font color
//								G_TYPE_STRING,	// Background color
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
													   "foreground", COLUMN_FONT_COLOR,
//													   "cell-background", COLUMN_BACK_COLOR,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("#", renderer,
													   "text", COLUMN_NUMBER,
													   "foreground", COLUMN_FONT_COLOR,
//													   "cell-background", COLUMN_BACK_COLOR,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("From"), renderer,
													   "text", COLUMN_SENDER,
													   "foreground", COLUMN_FONT_COLOR,
//													   "cell-background", COLUMN_BACK_COLOR,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Subject"), renderer,
													   "text", COLUMN_SUBJECT,
													   "foreground", COLUMN_FONT_COLOR,
//													   "cell-background", COLUMN_BACK_COLOR,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Date"), renderer,
													   "text", COLUMN_DATE,
													   "foreground", COLUMN_FONT_COLOR,
//													   "cell-background", COLUMN_BACK_COLOR,
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

	// That's it
	return true;
}


/** 
 * Update popup list
 *   Be careful that we're responsible for freeing memory of updated
 *   field within tree store. Easy solution is to collect every (gchar
 *   *) used in tree store and to free them next time we enter this
 *   function (saved_strings)
 **/
void
Popup::update (void)
{
	GtkListStore *store;
	GtkTreeIter iter;
	gchar *buffer;
	static std::vector <gchar *> saved_strings;
  
	// Get tree store and clear it
	store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW ((get("treeview")))));
	gtk_list_store_clear (store);

	// Free memory of previous strings displayed in popup
	for (guint i=0; i<saved_strings.size(); i++)
		g_free (saved_strings[i]);

	saved_strings.clear();

	unsigned int displayed_header = 0;
	// Now we populate list
	for (unsigned int j=0; j<biff_->size(); j++) {
		for (guint i=0; (i<biff_->mailbox(j)->unreads()); i++) {
			if (displayed_header < biff_->popup_max_line_) {
				std::stringstream s;
				s << i+1;
				gtk_list_store_append (store, &iter);
				
				// Subject
				buffer = parse_header (biff_->mailbox(j)->unread(i).subject);
				gchar *subject;
				subject = g_strndup (buffer, biff_->popup_max_subject_size_);
				g_free (buffer);
				saved_strings.push_back (subject);

				// Date
				gchar *date = parse_header (biff_->mailbox(j)->unread(i).date);
				saved_strings.push_back (date);
				
				// Sender
				gchar *buffer = parse_header (biff_->mailbox(j)->unread(i).sender);
				gchar *sender = g_strndup (buffer, biff_->popup_max_sender_size_);
				g_free (buffer);
				saved_strings.push_back (sender);

				if (i == 0)
					gtk_list_store_set (store, &iter, COLUMN_NAME, biff_->mailbox(j)->name().c_str(), -1);
				else
					gtk_list_store_set (store, &iter, COLUMN_NAME, "", -1);
				gtk_list_store_set (store, &iter,
									COLUMN_NUMBER, s.str().c_str(),
									COLUMN_SENDER, sender, 
									COLUMN_SUBJECT, subject,
									COLUMN_DATE, date,
									COLUMN_FONT_COLOR, biff_->popup_font_color_.c_str(),
//									COLUMN_BACK_COLOR, biff_->popup_back_color_.c_str(),
									COLUMN_HEADER, &biff_->mailbox(j)->unread(i),
									-1);
				displayed_header++;
			}
		}
	}

	// Update window decoration
	gtk_window_set_decorated (GTK_WINDOW(get("dialog")), biff_->popup_is_decorated_);

	// Update fonts
	GtkWidget *treeview = get("treeview");
	PangoFontDescription *font;
	font = pango_font_description_from_string (biff_->popup_font_.c_str());
	gtk_widget_modify_font (treeview, font);
	pango_font_description_free (font);

	// Update date display
	GtkTreeViewColumn *column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), COLUMN_DATE);
	gtk_tree_view_column_set_visible (column, biff_->popup_display_date_);
}

void
Popup::show (std::string name)
{
	for (unsigned int i=0; i<biff_->size(); i++)
		biff_->mailbox(i)->watch_off();

	tree_selection_ = 0;
	selected_header_ = 0;
	consulting_ = false;

	gtk_window_present (GTK_WINDOW(get("dialog")));
	if (biff_->popup_use_geometry_)
		gtk_window_parse_geometry (GTK_WINDOW(get("dialog")), biff_->popup_geometry_.c_str());

	g_static_mutex_lock (&timer_mutex_);
	if (poptag_ > 0) 
		g_source_remove (poptag_);
	poptag_ = g_timeout_add (biff_->popup_time_*1000, POPUP_on_popdown, this);
	g_static_mutex_unlock (&timer_mutex_);

	if (tree_selection_)
		gtk_tree_selection_unselect_all (tree_selection_);
}


gboolean
Popup::on_delete (GtkWidget *widget,
				  GdkEvent *event)
{
	hide ();
	g_static_mutex_lock (&timer_mutex_);
	if (poptag_ > 0) 
		g_source_remove (poptag_);
	poptag_ = 0;
	g_static_mutex_unlock (&timer_mutex_);

	if (biff_->check_mode_ == AUTOMATIC_CHECK)
		for (unsigned int i=0; i<biff_->size(); i++)
			biff_->mailbox(i)->watch_on();
	return true;
}

gboolean
Popup::on_popdown (void)
{
	hide();
	gtk_widget_hide(get("popup"));
	consulting_ = false;
	g_static_mutex_lock (&timer_mutex_);
	poptag_ = 0;
	g_static_mutex_unlock (&timer_mutex_);
	if (biff_->check_mode_ == AUTOMATIC_CHECK)
		for (unsigned int i=0; i<biff_->size(); i++)
			biff_->mailbox(i)->watch_on();
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
	else if (event->button == 3) {
		g_static_mutex_lock (&timer_mutex_);
		if (poptag_ > 0)
			g_source_remove (poptag_);
		poptag_ = 0;
		g_static_mutex_unlock (&timer_mutex_);
		hide ();
		gtk_widget_hide (get("popup"));
		consulting_ = false;
		if (biff_->check_mode_ == AUTOMATIC_CHECK)
			for (unsigned int i=0; i<biff_->size(); i++)
				biff_->mailbox(i)->watch_on();
	}
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
	g_static_mutex_lock (&timer_mutex_);
	if (poptag_ > 0)
		g_source_remove (poptag_);
	poptag_ = 0;
	g_static_mutex_unlock (&timer_mutex_);
}


void
Popup::on_leave (GdkEventCrossing *event)
{
	if (!consulting_) {
		g_static_mutex_lock (&timer_mutex_);
		if (poptag_ > 0)
			g_source_remove (poptag_);  
		poptag_ = g_timeout_add (biff_->popup_time_*1000, POPUP_on_popdown, this);
		g_static_mutex_unlock (&timer_mutex_);
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
		gtk_tree_model_get (model, &iter, COLUMN_HEADER, &address, -1);
		selected_header_ = (header *) address;
	}

	// If we're in consulting mode, update the text of the (single) mail popup
	if (consulting_ && selected_header_) {
		// Nop popdown when we're consulting an email
		g_static_mutex_lock (&timer_mutex_);
		if (poptag_ > 0)
			g_source_remove (poptag_);
		poptag_ = 0;
		g_static_mutex_unlock (&timer_mutex_);

		// Show popup window for mail displaying 
		// Name is stupid since we're in Popup 
		//  => "dialog" is the name of the real popup
		//  => "popup" is the name of the mail content window
		gtk_widget_show_all (get("popup"));
		gtk_window_move (GTK_WINDOW(get("popup")), x_, y_); 


		GtkWidget *view = get("textview");
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
		GtkTextIter iter;
		gtk_text_buffer_set_text (buffer, "", -1);
		gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);

		// Sender
		gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, _("From: "), -1, "bold", NULL);
		text = parse_header (selected_header_->sender);
		if (text) {
			gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, text, -1, "blue", NULL);
			g_free (text);
		}
		gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, "\n", -1, "normal", NULL);

		// Subject
		gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, _("Subject: "), -1, "bold", NULL);
		text = parse_header(selected_header_->subject);
		if (text) {
			gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, text, -1, "blue", NULL);
			g_free (text);
		}
		gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, "\n", -1, "normal", NULL);

		// Date
		gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, _("Date: "), -1, "bold",  NULL);
		text = parse_header(selected_header_->date);
		if (text) {
			gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, text, -1, "blue", NULL);
			g_free (text);
		}
		gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, "\n\n", -1, "normal", NULL);

		// Body
		text = convert (selected_header_->body, selected_header_->charset);
		if (text) {
			gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, text, -1, "normal", NULL);
			g_free (text);
		}
	}
}


/**
 *   Parse a header line to remove any quoted-printable or base64
 *  encoding. Subject line are kind of special because character set is
 *  encoded within the text For example it can be something like:
 *  =?iso-8859-1?Q?Apr=E8s?=
 **/
gchar *
Popup::parse_header (std::string text)
{
	// A mail header line (sender, subject or date) cannot contain
	// non-ASCII characters, so first, we remover any non-ASCII
	// character
	std::string copy;
	for (guint i=0; i<text.size(); i++)
		if (text[i] >= 0)
			copy += text[i];

	gchar *utf8_text = g_locale_to_utf8 ("", -1, 0, 0, 0);
	gchar *utf8_part = 0;
	std::string copy_part;
	std::string charset;
	char encoding = 0;
	gchar *buffer = 0;

	// Now we can begin translation
	guint i=0;
	do {
		// Charset description (=?iso-ABCD-XY?)
		if (copy.substr(i,2) == "=?") {
			// First concatenate the part we got so far (using locale charset)
			if (copy_part.size() > 0) {
				utf8_part = g_locale_to_utf8 (copy_part.c_str(), -1, 0, 0, 0);
				if (utf8_part) {
					buffer = g_strconcat (utf8_text, utf8_part, NULL);
					g_free (utf8_text);
					g_free (utf8_part);
					utf8_text = buffer;
				}
				copy_part.clear();
			}
			i+=2; 
			if (i >= copy.size()) {
				copy_part = _("* error *");
				break;
			}

			// Charset description
			while ((i < copy.size()) && (copy[i] != '?'))
				charset += copy[i++];
			i++;
			// End of charset description

			// Encoding description (Q or B. Others ?)
			if (i >= copy.size()) {
				copy_part = _("* error *");
				break;
			}
			encoding = copy[i++];
			i++;
			// End of encoding description

			// First, get (part of) the encoded string
			if (i >= copy.size()) {
				copy_part = _("* error *");
				break;
			}
			copy_part.clear();
			while ((i < copy.size()) && (copy.substr(i,2) != "?="))
				copy_part += copy[i++];
			if (i >= copy.size()) {
				copy_part = _("* error *");
				break;
			} 

			// Allocate a buffer to get the decoded string (whose size will be smaller)
			buffer = (gchar *) g_malloc (copy_part.size()+1);

			// Now decode
			if (copy_part.size() == 0)
				utf8_part = g_locale_to_utf8 ("", -1, 0, 0, 0);
			else if ((encoding == 'Q') || (encoding == 'q')) {
				decode_quoted (copy_part.c_str(), buffer);
				utf8_part = g_convert (buffer, -1, "utf-8", charset.c_str(), 0,0,0);
			}
			else if ((encoding == 'B') || (encoding == 'b')) {
				int size = decode_base64 ((gchar *) copy_part.c_str(), buffer);
				if (size > 0)
					utf8_part = g_convert (buffer, size, "utf-8", charset.c_str(), 0,0,0);
			}	
			else {
				utf8_part = g_locale_to_utf8 (copy_part.c_str(), -1, 0, 0, 0);
			}
			g_free (buffer);
			i += 2;
			// We translate to utf8 what we got
			if (utf8_part) {
				buffer = g_strconcat (utf8_text, utf8_part, NULL);
				g_free (utf8_text);
				g_free (utf8_part);
				charset = "";
				utf8_text = buffer;
			}
			copy_part = "";
		}
		// Normal text
		else
			copy_part += copy[i++];
	} while (i < copy.size());

	// Last (possible) part
	utf8_part = g_locale_to_utf8 (copy_part.c_str(), -1, 0, 0, 0);
	if (utf8_part) {
		buffer = g_strconcat (utf8_text, utf8_part, NULL);
		g_free (utf8_text);
		g_free (utf8_part);
		utf8_text = buffer;
	}

	return utf8_text;
}


int
Popup::decode_quoted (const gchar *buftodec,
					  gchar *decbuf)
{
	guint j=0;
	guint i=0;
	do {
		// Quoted printable code
		if ((buftodec[i] == '=') && ((i+2) < strlen(buftodec))) {
			char encoded[5];
			int decoded;
			encoded[0] = '0';
			encoded[1] = 'x';
			encoded[2] = buftodec[i+1];
			encoded[3] = buftodec[i+2];
			encoded[4] = '\0';
			sscanf (encoded, "%i", &decoded);
			decbuf[j++] = char (decoded);
			i += 3;
		}
		// End of quoted printable code

		// Normal text
		else if (buftodec[i] == '_') {
			decbuf[j++] = ' ';
			i++;
		}
		else
			decbuf[j++] = buftodec[i++];
	} while (i<strlen (buftodec));

	decbuf[j] = '\0';

	return strlen (decbuf);
}

int
Popup::decode_base64 (gchar *src, gchar *dest)
{
	int index_64[128] =	{
		-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
		52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-1,-1,-1,
		-1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
		15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
		-1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
		41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
	};
	gchar *end = (gchar *)(src + strlen(src));
	int size = 0;
	while (src + 3 < end) {
        *dest++ = (BASE64(src[0]) << 2) | (BASE64(src[1]) >> 4);
		size++;
		if (src[2] == '=')
			break;
		*dest++ = ((BASE64(src[1]) & 0xf) << 4) | (BASE64(src[2]) >> 2);
		size++;
		if (src[3] == '=')
			break;
		*dest++ = ((BASE64(src[2]) & 0x3) << 6) | BASE64(src[3]);
		size++;
		src += 4;
	}
	return size;
}


gchar *
Popup::convert (std::string text, std::string charset)
{
	gchar *utf8;

	if (!charset.empty())
		utf8 = g_convert (text.c_str(), -1, "utf-8", charset.c_str(), 0,0,0);
	else
		utf8 = g_locale_to_utf8 (text.c_str(), -1, 0, 0, 0);

	if (!utf8)
		utf8 = g_locale_to_utf8 (_("Error"), -1, 0, 0, 0);

	return utf8;
}
