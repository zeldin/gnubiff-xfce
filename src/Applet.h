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

#ifndef _APPLET_H
#define _APPLET_H

#ifdef HAVE_CONFIG_H
#   include "../config.h"
#endif
#include "GUI.h"
#include "Biff.h"

class Applet : public GUI {

	// ===================================================================
	// - Public methods --------------------------------------------------
	// ===================================================================
public:
	Applet (class Biff *owner, std::string xmlFilename);
	virtual ~Applet (void);
	void watch (void);						// Force a check for new mail now
	void watch_now (void);					// Check for new mail now
	void watch_on (void);					// Start watch on all available mailboxes
	void watch_off (void);					// Stop watch on all available mailboxes
	void process (void);					// Process current mailboxes

	guint unread_markup (std::string &text);// Return unread markup string
	std::string Applet::tooltip_text (void);// Return tooltip text

	virtual void update (void) = 0;			// Update applet
	virtual void dock (GtkWidget *applet) {};


	// ===================================================================
	// - Protected attributes --------------------------------------------
	// ===================================================================
protected:
	GMutex *			_process_mutex;	// Mutex for process function
	GMutex *			_update_mutex;	// Mutex for update function
	class Biff *		_owner;			// Owner of this interface
	gboolean			_force_popup; 	// Force popup required ?
};

#endif
