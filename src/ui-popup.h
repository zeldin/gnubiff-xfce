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

#ifndef __POPUP_H__
#define __POPUP_H__

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif
#include "gui.h"
#include "biff.h"

enum {
	COLUMN_NAME = 0,
	COLUMN_NUMBER,
	COLUMN_SENDER,
	COLUMN_SUBJECT,
	COLUMN_DATE,
	COLUMN_FONT_COLOR,
//	COLUMN_BACK_COLOR,
	COLUMN_HEADER,
	COLUMNS
};

#define BASE64(c) (index_64[(unsigned char)(c) & 0x7F])


class Popup : public GUI {

protected:
	class Biff *		biff_;				// Biff owner
	gint				poptag_;			// Tag for pop timer
	static GStaticMutex	timer_mutex_;		// Mutex for timer tag access
	header *			selected_header_;	// Current selected header
	GtkTreeSelection *	tree_selection_;	// Current tree selection
	gboolean			consulting_;		// Tag to know if we're consulting a mail
	gint				x_;					// Last mouse x position known
	gint				y_;					// Last mouse y position known


 public:
	/* base */
	Popup (class Biff *biff);
	virtual ~Popup (void);

	/* main */
	virtual gint create (void);
	void update (void);
	void show (std::string name = "dialog");

	/* callbacks */
	gboolean on_delete (GtkWidget *widget, GdkEvent *event);
	gboolean on_popdown (void);
	gboolean on_button_press (GdkEventButton *event);
	gboolean on_button_release (GdkEventButton *event);
	void on_enter (GdkEventCrossing *event);
	void on_leave (GdkEventCrossing *event);
	void on_select (GtkTreeSelection *selection);

private:
	/* util */
	gchar *parse_header (std::string text);
	int decode_quoted (const gchar *buftodec, gchar *decbuf);
	int decode_base64 (gchar *src, gchar *dest);
	gchar *convert (std::string text, std::string charset);
};

#endif
