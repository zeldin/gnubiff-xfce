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
#ifndef _POPUP_H
#define _POPUP_H

#ifdef HAVE_CONFIG_H
#   include "../config.h"
#endif
#include "GUI.h"
#include "Biff.h"



enum {
	COLUMN_NAME = 0,
	COLUMN_NUMBER,
	COLUMN_SENDER,
	COLUMN_SUBJECT,
	COLUMN_DATE,
	COLUMN_FONT_COLOR,
	COLUMN_BACK_COLOR,
	COLUMN_HEADER,
	COLUMNS
};

class Popup : public GUI {

	// ===================================================================
	// - Public methods --------------------------------------------------
	// ===================================================================
 public:
	Popup (class Biff *owner, std::string xmlFilename);
	virtual ~Popup (void);
	virtual gint create (void);
	void update (void);
	void show (std::string name = "dialog");

	gboolean on_delete (GtkWidget *widget, GdkEvent *event);
	gboolean on_popdown (void);
	gboolean on_button_press (GdkEventButton *event);
	gboolean on_button_release (GdkEventButton *event);
	void on_enter (GdkEventCrossing *event);
	void on_leave (GdkEventCrossing *event);
	void on_select (GtkTreeSelection *selection);


	// ===================================================================
	// - Protected attributes --------------------------------------------
	// ===================================================================
 protected:


	// ===================================================================
	// - Private methods -------------------------------------------------
	// ===================================================================
 private:
	gchar *parse_header (std::string text);
	int decode_quoted (const gchar *buftodec, gchar *decbuf);
	int decode_base64 (const gchar *buftodec, gchar *decbuf);
	gchar *convert (std::string text, std::string charset);


	// ===================================================================
	// - Private attributes ----------------------------------------------
	// ===================================================================
 private:
	class Biff *		_owner;				// Owner of this interface
	gint				_poptag;			// Tag for pop timer
	static GStaticMutex	_timer_mutex;		// Mutex for timer tag access
	header *			_selected_header;	// Current selected header
	GtkTreeSelection *	_tree_selection;	// Current tree selection
	gboolean			_consulting;		// Tag to know if we're consulting a mail
	gint				_x;					// Last mouse x position known
	gint				_y;					// Last mouse y position known
};

#endif
