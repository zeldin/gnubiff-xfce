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
#ifndef _GUI_H
#define _GUI_H

#ifdef HAVE_CONFIG_H
#   include "../config.h"
#endif
#include <string>
#include <glib.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

class GUI {

	// ===================================================================
	// - Public methods --------------------------------------------------
	// ===================================================================
 public:
	GUI (std::string xmlFilename);
	virtual ~GUI (void);

	virtual gint create (void);
	virtual void show (std::string name = "dialog");
	virtual void hide (std::string name = "dialog");

	std::string utf8_to_filename (std::string text);
	std::string utf8_to_locale (std::string text);
	std::string filename_to_utf8 (std::string text);
	std::string locale_to_utf8 (std::string text);

	GtkWidget *	get (std::string name);

	virtual gboolean on_delete (GtkWidget *widget,  GdkEvent *event);
	virtual gboolean on_destroy (GtkWidget *widget, GdkEvent *event);


	// ===================================================================
	// - Protected attributes --------------------------------------------
	// ===================================================================
 protected:
	GladeXML *		_xml;			// Interface description using XML/glade-2 file
	std::string		_xmlFilename;	// Name of the interface file (glade-2 format)
};

#endif
