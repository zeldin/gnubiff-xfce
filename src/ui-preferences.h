// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2007 Nicolas Rougier, 2004-2007 Robert Sowada
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

#ifndef __UI_PREFERENCES_H__
#define __UI_PREFERENCES_H__

#include "gui.h"
#include "ui-properties.h"

enum {
  COLUMN_UIN,
  COLUMN_MAILBOX_STOCK_ID,
  COLUMN_SSL_ICON,
  COLUMN_MAILBOX,
  COLUMN_FORMAT,
  COLUMN_STATUS_STOCK_ID,
  COLUMN_SECURITY_STOCK_ID,
  N_COLUMNS
};

enum { COL_EXP_ID, COL_EXP_NAME, COL_EXP_GROUPNAME, COL_EXP_TYPE,
	   COL_EXP_VALUE, COL_EXP_NAME_ITALIC, COL_EXP_EDITABLE, COL_EXP_N};

#define PREFERENCES(x)	(static_cast<Preferences *>(x))


class Preferences : public GUI {

protected:
	class Biff *		biff_;			// biff
	class Mailbox *		selected_;		// current selected mailbox
	class Mailbox *		added_;			// last added mailbox
	class Properties *	properties_;	// properties ui


public:
	/* Base */
	Preferences (class Biff *biff);
	~Preferences (void);
	gint create (gpointer callbackdata);

	/* Main */
	void show (std::string name = "dialog");
	void hide (std::string name = "dialog");
	void synchronize (void);
	void synchronize (class Mailbox *mailbox, GtkListStore *store, GtkTreeIter *iter);
	void apply (void);

	/* Access */
	class Properties *properties (void)		{return properties_;}
	class Biff * biff (void) 				{return biff_;}
	class Mailbox *added (void)				{return added_;}
	void added (class Mailbox *mailbox)		{added_ = mailbox;}
	class Mailbox *selected (void)			{return selected_;}
	void selected (class Mailbox *mailbox)	{
		selected_ = mailbox;
		if (properties_)
			properties_->select (mailbox);
	}

	/* Callbacks */
	void on_add					(GtkWidget *widget);
	void on_remove				(GtkWidget *widget);
	void on_properties			(GtkWidget *widget);
	void on_stop				(GtkWidget *widget);
	void on_close				(GtkWidget *widget);
	void on_browse_newmail_image(GtkWidget *widget);
	void on_browse_nomail_image	(GtkWidget *widget);
	void on_selection			(GtkTreeSelection *selection);
	void on_check_changed		(GtkWidget *widget);
	gboolean on_destroy			(GtkWidget *widget,
								 GdkEvent *event);
	gboolean on_delete			(GtkWidget *widget,
								 GdkEvent *event);

	/* Expert dialog */
	void expert_create (void);
	void expert_add_option_list (void);
	void expert_toggle_option (void);
	void expert_update_option_list (void);
	void expert_edit_value (void);
	void expert_update_option (const gchar *name, class Options *options,
							   GtkTreeIter *iter);
	void expert_set_selected_option (const gchar *new_text);
	void expert_on_selection (GtkTreeSelection *selection);
	gboolean expert_show_context_menu (GdkEventButton *event);
	void expert_search (void);
protected:
	gboolean expert_get_option (class Options *&options,class Option *&option);
	gboolean expert_get_option (class Options *&options, class Option *&option,
								GtkTreeIter &treeiter);

	// Treeview widget for the expert option editing dialog
	GtkTreeView *expert_treeview;
	// Liststore for the options in the expert option editing dialog
	GtkListStore *expert_liststore;
	// Treeview column that contains the values in the expert option dialog
	GtkTreeViewColumn *expert_col_value;
	// Textview for displaying the description of the currently selected option
	GtkTextView *expert_textview;
	// Buffer of the description of the currently selected option
	GtkTextBuffer *expert_textbuffer;
};

#endif
