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

#include "gtk_image_animation.h"
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
 *  Get the number of unread messages in all mailboxes.
 *
 *  @return    Number of unread messages.
 */
guint 
Applet::get_number_of_unread_messages (void)
{
	guint unread = 0;

	for (unsigned int i=0; i<biff_->size(); i++)
		unread += biff_->mailbox(i)->unreads();

	return unread;
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
	for (unsigned int i=0; i<biff_->size(); i++) {
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
		std::stringstream s;
		s << std::setfill('0') << std::setw (smax.str().size())
		  << biff_->mailbox(i)->unreads();
		// Checking mailbox?
		if (biff_->mailbox(i)->status() == MAILBOX_CHECK) {
			tooltip += "(";
			tooltip += s.str();
			tooltip += ")";
			tooltip += _(" checking...");
			continue;
		}
		// More unread messages in mailbox than got by gnubiff?
		if (biff_->mailbox(i)->unreads() >= biff_->value_uint ("max_mail")) {
			tooltip += std::string(smax.str().size(), '+');
			continue;
		}
		// Just the number of unread messages
		tooltip += s.str();
	}
	return tooltip;
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

// FIXME
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

/**
 *  Update the applet status. This includes showing the
 *  image/animation that corresponds to the current status of gnubiff
 *  (no new messages or new messages are present). Also the text with
 *  the current number of new messages is updated. If present a container
 *  widget that contains the widgets for the image and the text may be
 *  updated too.
 *
 *  @param  no_popup         If true the popup window will remain unchanged.
 *                           The default is false.
 *  @param  widget_image     Name of the widget that contains the image for
 *                           gnubiff's status or the empty string if
 *                           no image shall * be updated. The default
 *                           is the empty string.
 *  @param  widget_text      Name of the widget that contains the text for
 *                           gnubiff's status or the empty string if
 *                           no text shall be updated. The default is
 *                           the empty string.
 *  @param  widget_container Name of the widget that contains the image and
 *                           text widget. If it's an empty string the container
 *                           widget (if present) will not be updated. The
 *                           default is the empty string.
 *  @param  m_width          Maximum width of the widgets. The default value
 *                           is G_MAXUINT.
 *  @param  m_height         Maximum height of the widgets. The default value
 *                           is G_MAXUINT.
 */
void 
AppletGUI::update (gboolean no_popup, std::string widget_image,
				   std::string widget_text, std::string widget_container,
				   guint m_width, guint m_height)
{
	// Update applet's status: GUI-independent things to do
	Applet::update (no_popup);

	// Get number of unread messages
	guint unread = get_number_of_unread_messages ();

	// Update applet's image
	GtkWidget *widget = NULL;
	guint i_height = 0, i_width = 0;
	if (widget_image != "") {
		GtkImageAnimation *anim;
		anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("image")),
														"_animation_");
		widget = get (widget_image.c_str ());

		// Determine image
		std::string image;
		if ((unread == 0) && biff_->value_bool ("use_nomail_image"))
			image = biff_->value_string ("nomail_image");
		if ((unread >  0) && biff_->value_bool ("use_newmail_image"))
			image = biff_->value_string ("newmail_image");

		// Show/hide image
		if (image != "") {
			anim->open (image);
			// Get image's current size
			i_width = anim->scaled_width();
			i_height = anim->scaled_height();
			// Rescale image if it's too large
			gboolean rescale = false;
			if (i_width > m_width) {
				i_height = i_height * m_width / i_width;
				i_width  = m_width;
				rescale  = true;
			}
			if (i_height > m_height) {
				i_width  = i_width * m_height / i_height;
				i_height = m_height;
				rescale  = true;
			}
			if (rescale)
				anim->resize (i_width, i_height);
			// Start animation in updated widget
			gtk_widget_set_size_request (widget, i_width, i_height);
			gtk_widget_show (widget);
			anim->start();
		}
		else// Hide widget
			gtk_widget_hide (widget);
	}

	// Update applet's text
	GtkLabel *label = NULL;
	guint t_height = 0, t_width = 0;
	if (widget_text != "") {
		std::string text;
		label = GTK_LABEL (get (widget_text.c_str ()));

		unread_markup (text); // FIXME: Cleanup function
		gtk_label_set_markup (label, text.c_str());

		if (((unread == 0) && biff_->value_bool ("use_nomail_text")) ||
			((unread >  0) && biff_->value_bool ("use_newmail_text"))) {
			// Show widget	
			gtk_widget_show (GTK_WIDGET (label));

			// Resize label
			gtk_widget_set_size_request (GTK_WIDGET (label), -1, -1);
			GtkRequisition req;
			gtk_widget_size_request (GTK_WIDGET (label), &req);
			t_width = req.width;
			t_height = req.height;
		}
		else// Hide widget
			gtk_widget_hide (GTK_WIDGET (label));
	}

	// Update the container widget
	guint c_width = 0, c_height = 0;
	if (widget_container != "") {
		GtkFixed *fixed = GTK_FIXED (get (widget_container.c_str ()));

		// Calculate size of container: Image and text should fit into it.
		c_width = std::max (i_width, t_width);
		c_height = std::max (i_height, t_height);

		// Resize container and move widgets inside it to the right position
		if ((c_width > 0) && (c_height > 0)) {
			gtk_widget_set_size_request (GTK_WIDGET(fixed), c_width, c_height);
			if (label)
				gtk_fixed_move (fixed, GTK_WIDGET (label), (c_width-t_width)/2,
								c_height-t_height);
			if (widget)
				gtk_fixed_move (fixed, widget, (c_width-i_width)/2, 0);
		}
	}
}

/**
 *  Show the preferences dialog. Monitoring of the mailboxes will be stopped.
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
 *  Hide the preferences dialog. Monitoring of the mailboxes will be started
 *  again (if automatic checking is enabled).
 */
void 
AppletGUI::hide_dialog_preferences (void)
{
	// Hide the preferences dialog
	biff_->preferences()->hide();

	// Start monitoring of the mailboxes (if wanted)
	if (biff_->value_uint ("check_mode") == AUTOMATIC_CHECK)
		start (3);

	// Update applet's status
	update (true);
	show();
}

/**
 *  Show the about dialog.
 */
void 
AppletGUI::show_dialog_about (void)
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
AppletGUI::hide_dialog_about (void)
{
	GUI::hide ("about");
}
