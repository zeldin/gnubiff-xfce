// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2006 Nicolas Rougier, 2004-2006 Robert Sowada
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
// Author(s)     : Nicolas Rougier, Robert Sowada
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

/**
 *  Constructor.
 *
 *  @param  biff  Pointer to the biff object of the current gnubiff session.
 */
Applet::Applet (Biff *biff)
{
	biff_ = biff;
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
 *  Start the applet.
 *
 *  @param  showpref  If true and supported by the frontend the preferences
 *                    dialog is shown before monitoring starts (the default is
 *                    false).
 */
void 
Applet::start (gboolean showpref)
{
	if (!showpref)
		if (biff_->value_uint ("check_mode") == AUTOMATIC_CHECK)
			biff_->start_monitoring (3);
}

/**
 *  Update the applet status. If new messages are present the new
 *  mail command is executed.
 *
 *  @param  init  True if called when initializing gnubiff (the default is
 *                false)
 *  @return       True if new messages are present
 */
gboolean 
Applet::update (gboolean init)
{
#ifdef DEBUG
	g_message ("Applet update");
#endif

	// Check if there is new mail
	guint unread = 0;
	gboolean newmail = biff_->get_number_of_unread_messages (unread);

	// New mail command
	if ((newmail == true) && (unread > 0))
		execute_command ("newmail_command", "use_newmail_command");

	// Messages have been displayed now
	biff_->messages_displayed ();

	return newmail;
}

/**
 *  Mark all messages from all mailboxes as read and update the applet
 *  status.  The config file is saved with the information which
 *  messages have been read.
 */
void 
Applet::mark_messages_as_read (void)
{
	// Mark messages as read
	biff_->mark_messages_as_read ();

	// Save config file (especially the seen messages)
	biff_->save ();

	// Update the applet status
	update ();
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
 *  Get the number of unread messages in all mailboxes as a string.
 *
 *  @return    Number of unread messages as a string.
 */
std::string 
Applet::get_number_of_unread_messages (void)
{
	std::stringstream smax, text_zero, text_num;
	std::string text;

	// Get number of unread messages
	guint unread;
	biff_->get_number_of_unread_messages (unread);

	// Zero padded number of unread messages
	smax << biff_->value_uint ("max_mail");
	text_zero << std::setfill('0') << std::setw (smax.str().size()) << unread;

	// Number of unread messages
	text_num << unread;

	// Use the correct user supplied message to create the text
	const std::string chars = "dD";
	std::vector<std::string> vec(2);
	vec[0] = text_zero.str (); // 'd'
	vec[1] = text_num.str ();  // 'D'
	if (unread == 0)
		text = substitute (biff_->value_string ("nomail_text"), chars, vec);
	else if (unread < biff_->value_uint ("max_mail"))
		text = substitute (biff_->value_string ("newmail_text"), chars, vec);
	else {
		vec[0] = std::string (std::string(smax.str().size(), '+'));
		vec[1] = "+";
		text = substitute (biff_->value_string ("newmail_text"), chars, vec);
	}

	return text;
}

/**
 *  Returns a string with a overview of the statuses of all
 *  mailboxes. Each mailbox's status is presented on a separate line,
 *  if there is no problem the number of unread messages is
 *  given. This text is suitable for displaying as a tooltip.
 *
 *  @return    String that contains the mailboxes' statuses.
 */
std::string 
Applet::get_mailbox_status_text (void)
{
	// Get max collected mail number in a stringstream
	//  just to have a default string size.
	std::stringstream smax;
	smax << biff_->value_uint ("max_mail");

	std::string tooltip;
	for (unsigned int i=0; i < biff_->get_number_of_mailboxes (); i++) {
		if (i > 0)
			tooltip += "\n";
		// Mailbox's name
		tooltip += biff_->mailbox(i)->name();
		tooltip += ": ";

		// No protocol?
		if (biff_->mailbox(i)->protocol() == PROTOCOL_NONE) {
			tooltip += _(" unknown");
			continue;
		}
		// Error?
		if (biff_->mailbox(i)->status() == MAILBOX_ERROR) {
			tooltip += _(" error");
			continue;
		}
		// Put number of unread messages in the current mailbox into a string
		guint unread = biff_->mailbox(i)->unreads();
		std::stringstream s;
		s << std::setfill('0') << std::setw (smax.str().size()) << unread;
		// Checking mailbox?
		if (biff_->mailbox(i)->status() == MAILBOX_CHECK) {
			tooltip += "(" + s.str() + ")" + _(" checking...");
			continue;
		}
		// More unread messages in mailbox than got by gnubiff?
		if (unread >= biff_->value_uint ("max_mail")) {
			tooltip += std::string(smax.str().size(), '+');
			continue;
		}
		// Just the number of unread messages
		tooltip += s.str();
	}
	return tooltip;
}

/**
 *  The return value indicates whether the applet wants the mailboxes to be
 *  monitored.
 *
 *  @return true, if the applet thinks monitoring the mailboxes is okay
 */
gboolean 
Applet::can_monitor_mailboxes (void)
{
	return true;
}

/**
 *  Enable or disable the popup dialog.
 *
 *  @param  enable Boolean that indicates whether to enable (if true) or
 *                 disable (if false) the popup.
 */
void 
Applet::enable_popup (gboolean enable)
{
	// Change the value
	biff_->value ("use_popup", enable);
}
