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

#ifndef __APPLET_H__
#define __APPLET_H__

#include "gui.h"
#include "biff.h"

/**
 *  Generic non-GUI code common for all types of applets.
 */ 
class Applet
{
protected:
	class Biff *		biff_;
	GMutex *			process_mutex_;
	GMutex *			update_mutex_;
	gboolean			force_popup_;
public:
	// ========================================================================
	//  base
	// ========================================================================
	Applet (class Biff *biff);
	virtual ~Applet (void);

	// ========================================================================
	//  main
	// ========================================================================
	void start (guint delay=0);
	void stop  (void);
	virtual void update (gboolean no_popup = false);
	void mark_mails_as_read (void);
	void execute_command (std::string option_command,
						  std::string option_use_command = "");
	std::string get_mailbox_status_text (void);
};

/**
 *  Generic GUI code common for all types of applets.
 */ 
class AppletGUI : public Applet, public GUI {

public:
	// ========================================================================
	//  base
	// ========================================================================
	AppletGUI (class Biff *biff, std::string filename);
	virtual ~AppletGUI (void);

	// ========================================================================
	//  main
	// ========================================================================
	virtual void dock (GtkWidget *applet) {};		// dock applet

	guint       unread_markup (std::string &text);	// build unread markup string

	void show_dialog_preferences (void);
	void hide_dialog_preferences (void);
	void show_dialog_about (void);
	void hide_dialog_about (void);
};

#endif
