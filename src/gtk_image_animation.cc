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
#include <sstream>
#include "gtk_image_animation.h"


// ===================================================================
// = Callbacks "C" binding ===========================================
// ===================================================================
extern "C" {
	gboolean GTK_IMAGE_ANIMATION_timeout (gpointer data)
	{ return ((GtkImageAnimation *) data)->timeout (); }
	gboolean GTK_IMAGE_ANIMATION_on_delete (GtkWidget *widget,  GdkEvent *event, gpointer data)
	{ return ((GtkImageAnimation *) data)->on_delete (); }
	gboolean GTK_IMAGE_ANIMATION_on_destroy (GtkWidget *widget,  GdkEvent *event, gpointer data)
	{ return ((GtkImageAnimation *) data)->on_destroy (); }
	void GTK_IMAGE_ANIMATION_on_hide (GtkWidget *widget,  gpointer data)
	{ ((GtkImageAnimation *) data)->on_hide (); }
	void GTK_IMAGE_ANIMATION_on_show (GtkWidget *widget,  gpointer data)
	{ ((GtkImageAnimation *) data)->on_show (); }
}


// ===================================================================
//  Constructor
// -------------------------------------------------------------------
//
// ===================================================================
GtkImageAnimation::GtkImageAnimation (GtkImage *image) {
	_image = image;
	_animation = 0;
	_original_pixbuf = 0;
	_scaled_pixbuf = 0;
	_frame.clear();
	_current = 0;
	_original_width = 0;
	_original_height = 0;
	_scaled_width = 0;
	_scaled_height = 0;
	_timetag = 0;
	_speed = 30; // 20 milliseconds is quite too short a delay
	_object_mutex = g_mutex_new();
	g_signal_connect (G_OBJECT (image), "delete-event", G_CALLBACK (GTK_IMAGE_ANIMATION_on_delete), this);  
	g_signal_connect (G_OBJECT (image), "destroy-event", G_CALLBACK (GTK_IMAGE_ANIMATION_on_destroy), this);  
	g_signal_connect (G_OBJECT (image), "hide", G_CALLBACK (GTK_IMAGE_ANIMATION_on_hide), this);  
	g_signal_connect (G_OBJECT (image), "show", G_CALLBACK (GTK_IMAGE_ANIMATION_on_show), this);  
}


// ===================================================================
//  Destructor
// -------------------------------------------------------------------
//
// ===================================================================
GtkImageAnimation::~GtkImageAnimation (void) {
	g_mutex_lock (_object_mutex);
	if (_timetag)
		g_source_remove (_timetag);
	if (_animation)
		g_object_unref (_animation);
	if (_original_pixbuf)
		g_object_unref (_original_pixbuf);
	if (_scaled_pixbuf)
		g_object_unref (_scaled_pixbuf);
	for (guint i=0; i<_frame.size(); i++)
		if (_frame[i])
			g_object_unref (_frame[i]);
	_frame.clear();
}


// ===================================================================
//  timeout
// -------------------------------------------------------------------
//
// ===================================================================
gboolean GtkImageAnimation::timeout (void) {
	gdk_threads_enter();
	if (!GTK_IS_IMAGE(_image)) {
		gdk_threads_leave();
		_timetag = 0;
		return false;
	}
	_current = (_current+1)%_frame.size();
	if (_frame.size())
		gtk_image_set_from_pixbuf (_image, _frame[_current]);
	gdk_threads_leave();
	return true;
}

// ===================================================================
//  open a new animation file
// -------------------------------------------------------------------
// 
// Since the only supported animation format for gdk-pixbuf seems to  
// be the gif one, I needed another one that allows more color and
// is not patented so some stupid company.
// First option was to use mng but it is not yet supported by gdk-pixbuf.
// Since I did not want to create a new format, I use instead a "normal"
// image where frames are on top of each other (convert command from
// ImageMagick with the -append option just do that). But, since I have
// no information on the possible animation, I have to use filename to
// to gather information about a single frame size.
//
// Example:
// --------
//  gnubiff is bundled with the png image "tux(64x64).png"
//  (It is unlikely that someone will name an image using parenthesis
//   so it's a good hint for us it may be an animation).
//  Here, the "64x64" means that one frame is 64 by 64 in size.
//  Furthermore, image size is 1024x64, so I can deduce that there
//  is 1024/64 = 16 frames within.
//  We're now able to play the animation by displaying subsequent frames
//  with a timeout callback. That's it !
// 
// ===================================================================
gboolean GtkImageAnimation::open (std::string filename) {
	g_mutex_lock (_object_mutex);
	// Stop animation
	stop ();

	// Try to load given file
	GdkPixbufAnimation *animation = gdk_pixbuf_animation_new_from_file (filename.c_str(), 0);
	
	// Check if file is valid
	if (!animation) {
		g_mutex_unlock (_object_mutex);
		return false;
	}
	
	// Is it already an animation ?
	//  (in this case, there not much to do)
	if (!gdk_pixbuf_animation_is_static_image (animation)) {
		// Set animation in image
		gtk_image_set_from_animation (_image, animation);
		// Free previous stuff
		if (_animation)	
			g_object_unref (_animation);
		_animation = 0;
		if (_original_pixbuf)
			g_object_unref (_original_pixbuf);
		_original_pixbuf = 0;
		if (_scaled_pixbuf)	
			g_object_unref (_scaled_pixbuf);
		_scaled_pixbuf = 0;
		for (guint i=0; i<_frame.size(); i++)
			if (_frame[i])
				g_object_unref (_frame[i]);
		_frame.clear();
		// Get new animation & filename
		_animation = animation;
		_original_width  = gdk_pixbuf_animation_get_width (_animation);
		_original_height = gdk_pixbuf_animation_get_height (_animation);
		_scaled_width = _original_width;
		_scaled_height = _original_height;
		_filename = filename;
		g_mutex_unlock (_object_mutex);
		return true;
	}
	

	// Ok, we have a static image, so we get it and delete animation
	//  (we won't use it anymore)
	if (_original_pixbuf)
		g_object_unref (_original_pixbuf);
	_original_pixbuf = gdk_pixbuf_copy (gdk_pixbuf_animation_get_static_image (animation));
	g_object_unref (animation);
	if (_animation)
		g_object_unref (_animation);
	_animation = 0;

	// Ok, let's see what is the animation format

	_filename = filename;

	// It's like we got only one frame (no '('in filename)
	if (!is_animation()) {
		_original_width  = gdk_pixbuf_get_width (_original_pixbuf);
		_original_height = gdk_pixbuf_get_height (_original_pixbuf);
		_scaled_width  = _original_width;
		_scaled_height = _original_height;
		if (_scaled_pixbuf)
			g_object_unref (_scaled_pixbuf);
		_scaled_pixbuf = gdk_pixbuf_copy (_original_pixbuf);
		for (guint i=0; i<_frame.size(); i++)
			if (_frame[i])
				g_object_unref (_frame[i]);
		_frame.clear();
		gtk_image_set_from_pixbuf (_image, _scaled_pixbuf);
		g_mutex_unlock (_object_mutex);
		return true;
	}

	// Don't forget to set size to 0 since resize will check for size equality
	//  (and do nothing if original = scaled)
	_scaled_height = 0;
	_scaled_width  = 0;
	_current = 0;
	resize (_original_width, _original_height, true);
	g_mutex_unlock (_object_mutex);
	return true;
}


// ===================================================================
//  Animation start/stop
// -------------------------------------------------------------------
//
// ===================================================================
void GtkImageAnimation::resize (guint width, guint height, gboolean locked) {
	// Do we have something to resize ?
	if (!_original_pixbuf)
		return;

	// Do we already got the right size ?
	if ((width == _scaled_width) && (height == _scaled_height))
		return;

	if (!locked)
		g_mutex_lock (_object_mutex);

	_scaled_width = width;
	_scaled_height = height;

	// Free previous scaled pixbuf & frames
	if (_scaled_pixbuf)
		g_object_unref (_scaled_pixbuf);

	// If we have only one frame...
	if (_original_width == (guint) gdk_pixbuf_get_width (_original_pixbuf)) {
		_scaled_pixbuf = gdk_pixbuf_scale_simple (_original_pixbuf, _scaled_width, _scaled_height, GDK_INTERP_BILINEAR);
		gtk_image_set_from_pixbuf (_image, _scaled_pixbuf);
		if (!locked)
			g_mutex_unlock (_object_mutex);
		return;
	}

	// Ok, we have a sequence to scale
	for (guint i=0; i<_frame.size(); i++)
		if (_frame[i])
			g_object_unref (_frame[i]);
	_frame.clear();
	guint n = gdk_pixbuf_get_width (_original_pixbuf)/_original_width;
	if (n == 0)
		n = 1;
	//  Actual rescale
	_scaled_pixbuf = gdk_pixbuf_scale_simple (_original_pixbuf, n*_scaled_width, _scaled_height, GDK_INTERP_BILINEAR);

	// Rebuild each frame
	for (guint i=0; i<n; i++) {
		GdkPixbuf *pixbuf = gdk_pixbuf_new_subpixbuf (_scaled_pixbuf, i*_scaled_width, 0, _scaled_width, _scaled_height);
		_frame.push_back (pixbuf);
	}

	gtk_image_set_from_pixbuf (_image, _frame[0]);
	if (!locked)
		g_mutex_unlock (_object_mutex);
}

// ===================================================================
//  Animation start/stop
// -------------------------------------------------------------------
//
// ===================================================================
void GtkImageAnimation::start (void) {
	if ((_frame.size() > 0) && (_timetag == 0) && (GTK_IS_IMAGE(_image)))
		_timetag = g_timeout_add (_speed, GTK_IMAGE_ANIMATION_timeout, this);
}
void GtkImageAnimation::stop (void) {
	if (_timetag)
		g_source_remove (_timetag);
	_timetag = 0;
}


// ===================================================================
//  Callbacks
// -------------------------------------------------------------------
//
// ===================================================================
gboolean GtkImageAnimation::is_animation (void) {
	// If there is no '(' in name, we consider to have a static image
	if (_filename.find('(') == std::string::npos)
		return false;
	
	guint i = _filename.find ('(')+1;
	std::string sw, sh;
	do {
		sw += _filename[i++];
	} while ((i<_filename.size()) && (g_ascii_isdigit(_filename[i])));

	if ((i<_filename.size()) && (_filename[i] != 'x'))
		return false;
	i++;
	do {
		sh += _filename[i++];
	} while ((i<_filename.size()) && (g_ascii_isdigit(_filename[i])));
	std::stringstream ssw(sw);
	std::stringstream ssh(sh);
	guint w, h;
	ssw >> w;
	ssh >> h;
	// Is the new "found height" equal to pixbuf height
	//  (frame must be horizontally next to each other
	//   so if a frame width is not pixbuf height, there is a problem
	if (h != (guint) gdk_pixbuf_get_height (_original_pixbuf))
		return false;

	// Do we have an interger number of frame ?
	if (gdk_pixbuf_get_width(_original_pixbuf)%w)
		return false;

	_original_width  = w;
	_original_height = h;
	return true;
}

// ===================================================================
//  Callbacks
// -------------------------------------------------------------------
//
// ===================================================================
gboolean GtkImageAnimation::on_delete (void) {
	stop();
	return true;
}
gboolean GtkImageAnimation::on_destroy (void) {
	stop();
	return true;
}
void GtkImageAnimation::on_hide (void) {
	stop();
}
void GtkImageAnimation::on_show (void) {
	start();
}
