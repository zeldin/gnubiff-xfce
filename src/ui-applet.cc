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

#include "support.h"

#include <sstream>
#include <iomanip>
#include <cstdio>
#include <string>
#include <glib.h>

#include "ui-applet.h"
#include "ui-popup.h"
#include "mailbox.h"


Applet::Applet (Biff *biff,
				std::string filename) : GUI (filename)
{
	biff_ = biff;
	force_popup_ = false;
	update_mutex_ = g_mutex_new ();
}

Applet::~Applet (void)
{
	g_mutex_lock (update_mutex_);
	g_mutex_unlock (update_mutex_);
	g_mutex_free (update_mutex_);
}

void Applet::start (guint delay)
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

void
Applet::stop (void)
{
#ifdef DEBUG
	g_message ("Stop monitoring mailboxes");
#endif
	for (unsigned int i=0; i<biff_->size(); i++)
		biff_->mailbox(i)->stop ();
}

guint
Applet::unread_markup (std::string &text)
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
		text += gb_substitute (biff_->value_string ("nomail_text"), "d", vec);
	}
	else if (unread < biff_->value_uint ("max_mail")) {
		vec[0] = std::string (unreads.str());
		text += gb_substitute (biff_->value_string ("newmail_text"), "d", vec);
	}
	else {
		vec[0] = std::string (std::string(smax.str().size(), '+'));
		text += gb_substitute (biff_->value_string ("newmail_text"), "d", vec);
	}
	text += "</span>";
	
	return unread;
}

std::string
Applet::tooltip_text (void)
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
		s << std::setfill('0') << std::setw (smax.str().size()) << biff_->mailbox(i)->unreads();

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


void
Applet::update (gboolean no_popup)
{
#ifdef DEBUG
	g_message ("Applet update");
#endif

	gboolean newmail = false;
	int unread = 0;
	for (unsigned int i=0; i<biff_->size(); i++) {
		guint status = biff_->mailbox(i)->status();

		if (status == MAILBOX_NEW)
			newmail = true;
		unread += biff_->mailbox(i)->unreads();
	}

	if ((!no_popup) && (newmail == true) && (unread > 0)
		&& (force_popup_ == false)
		&& (biff_->value_bool ("use_newmail_command"))) {
		std::string command = biff_->value_string ("newmail_command") + " &";
		system (command.c_str());
	}

	if (!no_popup && (biff_->popup())) {
		// If there are no mails to display then hide popup
		if (!unread && (biff_->value_bool ("use_popup") || force_popup_))
			biff_->popup()->hide();

		// Otherwise update and display the popup
		if (unread && ((biff_->value_bool ("use_popup") && newmail)
					   || (force_popup_))) {
			biff_->popup()->update();
			biff_->popup()->show();
		}
	}

	// Mail has been displayed now
	for (unsigned int i=0; i<biff_->size(); i++)
		biff_->mailbox(i)->mail_displayed ();

	force_popup_ = false;
}
