// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2005 Nicolas Rougier
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
#include <iomanip>
#include <cstdio>
#include <string>
#include <glib.h>

#include "mailbox.h"
#include "support.h"
#include "ui-applet.h"
#include "ui-popup.h"
#include "ui-preferences.h"

/**
 *  Constructor.
 *
 *  @param  biff  Pointer to the biff object of the current gnubiff session.
 */
Applet::Applet (Biff *biff)
{
	biff_ = biff;
	force_popup_ = false;
	update_mutex_ = g_mutex_new ();
}

/// Destructor
Applet::~Applet (void)
{
	g_mutex_lock (update_mutex_);
	g_mutex_unlock (update_mutex_);
	g_mutex_free (update_mutex_);
}

/**
 *  Start monitoring all mailboxes. Optionally the delay {\em delay} can be
 *  given, so monitoring starts later.
 *
 *  @param  delay  Delay in seconds (the default is 0).
 */
void 
Applet::start (guint delay)
{
#ifdef DEBUG
	if (delay)
		g_message ("Start monitoring mailboxes in %d second(s)", delay);
	else
		g_message ("Start monitoring mailboxes now");
#endif
	for (unsigned int i=0; i<biff_->size(); i++)
		biff_->mailbox(i)->threaded_start (delay);
}

/**
 *  Stop monitoring all mailboxes.
 */
void 
Applet::stop (void)
{
#ifdef DEBUG
	g_message ("Stop monitoring mailboxes");
#endif
	for (unsigned int i=0; i<biff_->size(); i++)
		biff_->mailbox(i)->stop ();
}

/**
 *  Update the applet status. If new messages are present the new
 *  mail command is executed. The status of the popup window is updated.
 *
 *  @param  no_popup  If true the popup window will remain unchanged.
 */
void 
Applet::update (gboolean no_popup)
{
#ifdef DEBUG
	g_message ("Applet update");
#endif

	// Check if there is new mail
	gboolean newmail = false;
	gint unread = 0;
	for (guint i=0; i<biff_->size(); i++) {
		guint status = biff_->mailbox(i)->status();

		if (status == MAILBOX_NEW)
			newmail = true;
		unread += biff_->mailbox(i)->unreads();
	}

	// New mail command
	if ((newmail == true) && (unread > 0) && (force_popup_ == false))
		execute_command ("newmail_command", "use_newmail_command");

	// Update popup
	if (!no_popup && (biff_->popup())) {
		// If there are no mails to display then hide popup
		if (!unread && (biff_->value_bool ("use_popup") || force_popup_))
			biff_->popup()->hide();

		// Test if the popup is visible. If it is visible we also have to
		// update when mails are read
		gboolean vis = GTK_WIDGET_VISIBLE (biff_->popup()->get ("dialog"));

		// Update and display the popup
		if (unread && ((biff_->value_bool ("use_popup")) || force_popup_)
			&& (newmail || vis || force_popup_)) {
			biff_->popup()->update();
			biff_->popup()->show();
		}
	}

	// Mail has been displayed now
	for (guint i=0; i < biff_->size(); i++)
		biff_->mailbox(i)->mail_displayed ();

	force_popup_ = false;
}

/**
 *  Mark all mails from all mailboxes as read and update the applet status.
 */
void 
Applet::mark_mails_as_read (void)
{
	// Mark mails as read
	for (unsigned int i=0; i<biff_->size(); i++)
		biff_->mailbox(i)->read();

	// Update the applet status
	//force_popup_ = true;
	//biff_->popup()->hide();
	update();
}

/**
 *  Execute a shell command that is stored in the biff string option
 *  {\em option_command} if the biff boolean option {\em option_use_command}
 *  is true.
 *
 *  @param option_command     Name of the biff string option that stores the
 *                            command.
 *  @param option_use_command Name of the biff boolean option that indicates
 *                            whether the command should be executed or not.
 *                            If this string is empty the command will be
 *                            executed (the default is the empty string).
 */
void 
Applet::execute_command (std::string option_command,
						 std::string option_use_command)
{
	// Shall the command be executed?
	if ((!option_use_command.empty()) &&
		(!(biff_->value_bool (option_use_command))))
		return;
	// Execute the command (if there is one).
	std::string command = biff_->value_string (option_command);
    if (!command.empty ()) {
		command += " &";
		system (command.c_str ());
	}
}


/**
 *  Constructor.
 *
 *  @param  biff     Pointer to the biff object of the current gnubiff session.
 *  @param  filename Name of the glade file that contains the GUI information.
 */
AppletGUI::AppletGUI (Biff *biff, std::string filename) : Applet (biff),
														  GUI (filename)
{
}

/// Destructor
AppletGUI::~AppletGUI (void)
{
}

guint 
AppletGUI::unread_markup (std::string &text)
{
	// Get max collected mail number in a stringstream
	//  just to have a default string size.
	std::stringstream smax;
	smax << biff_->value_uint ("max_mail");

	// Get number of unread mails
	guint unread = 0;
	for (unsigned int i=0; i<biff_->size(); i++)
		unread += biff_->mailbox(i)->unreads();
	std::stringstream unreads;
	unreads << std::setfill('0') << std::setw (smax.str().size()) << unread;

	// Applet label (number of mail)
	text = "<span font_desc=\"" + biff_->value_string ("applet_font") + "\">";

	std::vector<std::string> vec(1);
	if (unread == 0) {
		vec[0] = std::string (unreads.str());
		text += substitute (biff_->value_string ("nomail_text"), "d", vec);
	}
	else if (unread < biff_->value_uint ("max_mail")) {
		vec[0] = std::string (unreads.str());
		text += substitute (biff_->value_string ("newmail_text"), "d", vec);
	}
	else {
		vec[0] = std::string (std::string(smax.str().size(), '+'));
		text += substitute (biff_->value_string ("newmail_text"), "d", vec);
	}
	text += "</span>";
	
	return unread;
}

std::string
AppletGUI::tooltip_text (void)
{
	// Get max collected mail number in a stringstream
	//  just to have a default string size.
	std::stringstream smax;
	smax << biff_->value_uint ("max_mail");

	std::string tooltip;
	for (unsigned int i=0; i<biff_->size(); i++) {
		tooltip += biff_->mailbox(i)->name();
		tooltip += " : ";
		std::stringstream s;
		s << std::setfill('0') << std::setw (smax.str().size())
		  << biff_->mailbox(i)->unreads();

		if (biff_->mailbox(i)->protocol() == PROTOCOL_NONE)
			tooltip += _(" unknown");
		else if (biff_->mailbox(i)->status() == MAILBOX_ERROR)
			tooltip += _(" error");
		else if (biff_->mailbox(i)->status() == MAILBOX_CHECK) {
			tooltip += "(";
			tooltip += s.str();
			tooltip += ")";
			tooltip += _(" checking...");
		}
		else if (biff_->mailbox(i)->unreads() >= biff_->value_uint ("max_mail")) {
			tooltip += std::string(smax.str().size(), '+');
		}
		else
			tooltip += s.str();
		if (i< (biff_->size()-1))
			tooltip += "\n";
	}
	return tooltip;
}

/**
 *  Show the preferences dialog.
 */
void 
AppletGUI::show_dialog_preferences (void)
{
	// Hide the popup window
	biff_->popup()->hide();

	// Show the dialog
	biff_->preferences()->show();

	// Stop monitoring mailboxes
	stop ();
}


/**
 *  Hide the preferences dialog.
 */
void 
AppletGUI::hide_dialog_preferences (void)
{
	// Hide the dialog
	biff_->preferences()->hide();
}

/**
 *  Show the about dialog.
 */
void 
AppletGUI::show_about (void)
{
	// Hide the other dialogs
	biff_->popup()->hide();
	biff_->preferences()->hide();

	// Show the dialog
	GUI::show ("about");
}

/**
 *  Hide the about dialog.
 */
void 
AppletGUI::hide_about (void)
{
	GUI::hide ("about");
}
