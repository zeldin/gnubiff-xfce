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
#ifndef _GTK_IMAGE_ANIMATION_H
#define _GTK_IMAGE_ANIMATION_H

#ifdef HAVE_CONFIG_H
#   include "../config.h"
#endif
#include <string>
#include <vector>
#include <gtk/gtk.h>


class GtkImageAnimation {
	// ===================================================================
	// - Public methods --------------------------------------------------
	// ===================================================================
 public:
	GtkImageAnimation (GtkImage *image);
	~GtkImageAnimation (void);
	gboolean open (std::string filename);
	void resize (guint width, guint height, gboolean locked=false);
	void start (void);
	void stop (void);

	void attach (GtkImage *image)		{_image = image;};
	std::string filename (void)			{return _filename;};
	guint original_width (void)			{return _original_width;};
	guint original_height (void)		{return _original_height;};
	guint scaled_width (void)			{return _scaled_width;};
	guint scaled_height (void)			{return _scaled_height;};
	

	// ===================================================================
	// - Private methods -------------------------------------------------
	// ===================================================================
 public:
	gboolean timeout (void);
	gboolean on_delete (void);
	gboolean on_destroy (void);
	void on_show (void);
	void on_hide (void);
 private:
	gboolean is_animation (void);


	// ===================================================================
	// - Private attributes ----------------------------------------------
	// ===================================================================
 private:
	std::string					_filename;
	GtkImage *					_image;
	GdkPixbufAnimation *		_animation;
	GdkPixbuf *					_original_pixbuf;
	GdkPixbuf *					_scaled_pixbuf;
	std::vector<GdkPixbuf *>	_frame;
	gint						_current;
	guint						_original_width;
	guint						_original_height;
	guint						_scaled_width;
	guint						_scaled_height;
	gint						_timetag;
	guint						_speed;
	GMutex *					_object_mutex;
};

#endif
