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

#ifndef __GUI_H__
#define __GUI_H__

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif
#include <string>
#include <gtk/gtk.h>
#include <glade/glade.h>


class GUI {

protected:
	GladeXML *		xml_;		// interface description using XML/glade-2 file
	std::string		filename_;	// name of the interface file

public:
	/* base */
	GUI (std::string filename);
	virtual ~GUI (void);

	/* main */
	virtual gint create (void);
	virtual void show (std::string name = "dialog");
	virtual void hide (std::string name = "dialog");
	gboolean browse (std::string title,
					 std::string widget_name,
					 gboolean file_and_folder = false,
					 GtkWidget *widget = 0);

	/* access */
	GtkWidget *	get (std::string name = "dialog");
	std::string utf8_to_filename (std::string text);
	std::string utf8_to_locale (std::string text);
	std::string filename_to_utf8 (std::string text);
	std::string locale_to_utf8 (std::string text);

	/* frequent callbacks */
	virtual gboolean on_delete	(GtkWidget *widget, GdkEvent *event);
	virtual gboolean on_destroy	(GtkWidget *widget, GdkEvent *event);
	virtual void	 on_ok		(GtkWidget *widget) {};
	virtual void 	 on_apply	(GtkWidget *widget) {};
	virtual void	 on_close	(GtkWidget *widget) {};
	virtual void	 on_cancel	(GtkWidget *widget) {};
};

/* "C" bindings */
extern "C" {
	void GUI_connect (const gchar *handler_name,
					  GObject *object,
					  const gchar *signal_name,
					  const gchar *signal_data,
					  GObject *connect_object,
					  gboolean after,
					  gpointer user_data);

	void GUI_update_preview (GtkWidget *widget,
							 gpointer data);
}

#endif
