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

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <sstream>
#include <iomanip>
#include <cstdio>
#include <string>
#include <glib.h>

#include "ui-applet.h"
#include "ui-popup.h"
#include "mailbox.h"
#include "nls.h"


Applet::Applet (Biff *biff,
				std::string filename) : GUI (filename)
{
	biff_ = biff;
	force_popup_ = false;
	process_mutex_ = g_mutex_new ();
	update_mutex_ = g_mutex_new ();
}

Applet::~Applet (void)
{
	g_mutex_lock (process_mutex_);
	g_mutex_unlock (process_mutex_);
	g_mutex_free (process_mutex_);
	g_mutex_lock (update_mutex_);
	g_mutex_unlock (update_mutex_);
	g_mutex_free (update_mutex_);
}

void Applet::watch (void)
{
	force_popup_ = true;
	for (unsigned int i=0; i<biff_->size(); i++)
		biff_->mailbox(i)->watch();
}

void Applet::watch_now (void)
{
#ifdef DEBUG
	g_message ("Starting watching mailboxes");
#endif
	force_popup_ = false;
	for (unsigned int i=0; i<biff_->size(); i++) {
#ifdef DEBUG
		g_message ("[%d]: watch() function called", i+1);
#endif
		biff_->mailbox(i)->watch();
	}
}

void Applet::watch_on (guint delay)
{
#ifdef DEBUG
	g_message ("Watch on");
#endif
	for (unsigned int i=0; i<biff_->size(); i++)
		biff_->mailbox(i)->watch_on (delay);
}

void Applet::watch_off (void)
{
#ifdef DEBUG
	g_message ("Watch off");
#endif
	for (unsigned int i=0; i<biff_->size(); i++)
		biff_->mailbox(i)->watch_off();
}

guint Applet::unread_markup (std::string &text) {
	// Get max collected mail number in a stringstream
	//  just to have a default string size.
	std::stringstream smax;
	smax << biff_->max_mail_;

	// Get number of unread mails
	guint unread = 0;
	for (unsigned int i=0; i<biff_->size(); i++)
		unread += biff_->mailbox(i)->unreads();
	std::stringstream unreads;
	unreads << std::setfill('0') << std::setw (smax.str().size()) << unread;

	// Applet label (number of mail)

	text = "<span font_desc=\"";
	text += biff_->biff_font_;
	text += "\">";
	text += "<span color=\"";
	text += biff_->biff_font_color_;
	text += "\">";

	std::string ctext;
	if (unread == 0) {
		ctext = biff_->biff_nomail_text_;
		guint i;
		while ((i = ctext.find ("%d")) != std::string::npos) {
			ctext.erase (i, 2);
			ctext.insert(i, unreads.str());
		}
	}
	else if (unread < biff_->max_mail_) {
		ctext = biff_->biff_newmail_text_;
		guint i;
		while ((i = ctext.find ("%d")) != std::string::npos) {
			ctext.erase (i, 2);
			ctext.insert(i, unreads.str());
		}
	}
	else {
		ctext = biff_->biff_newmail_text_;
		guint i;
		while ((i = ctext.find ("%d")) != std::string::npos) {
			ctext.erase (i, 2);
			ctext.insert(i, std::string(smax.str().size(), '+'));
		}
	}
	text += ctext;
	text += "</span></span>";
	
	return unread;
}

// ================================================================================
//  Build the unread markup string
// --------------------------------------------------------------------------------
//  
// ================================================================================
std::string Applet::tooltip_text (void) {
	// Get max collected mail number in a stringstream
	//  just to have a default string size.
	std::stringstream smax;
	smax << biff_->max_mail_;

	std::string tooltip;
	for (unsigned int i=0; i<biff_->size(); i++) {
		tooltip += biff_->mailbox(i)->name();
		tooltip += " : ";
		std::stringstream s;
		s << std::setfill('0') << std::setw (smax.str().size()) << biff_->mailbox(i)->unreads();

		if (biff_->mailbox(i)->status() == MAILBOX_ERROR)
			tooltip += _("error");
		else if (biff_->mailbox(i)->status() == MAILBOX_BLOCKED)
			tooltip += _("blocked (unsecure)");
		else if (biff_->mailbox(i)->status() == MAILBOX_CHECKING) {
			tooltip += "(";
			tooltip += s.str();
			tooltip += ")";
			tooltip += _(" checking...");
		}
		else if (biff_->mailbox(i)->unreads() >= biff_->max_mail_) {
			tooltip += std::string(smax.str().size(), '+');
		}
		else
			tooltip += s.str();
		if (i< (biff_->size()-1))
			tooltip += "\n";
	}
	return tooltip;
}

// ================================================================================
//  Process mailbox
// --------------------------------------------------------------------------------
//  This function decide what to do with current mailboxes states
// ================================================================================
void Applet::process (void) {
	// Are we already updating applet ?
	if (!g_mutex_trylock (process_mutex_))
		return;
  
	// Look for status of mailboxes
	gboolean newmail = false;
	int unread = 0;
	for (unsigned int i=0; i<biff_->size(); i++) {
		if (biff_->mailbox(i)->status() == MAILBOX_NEW)
			newmail = true;
		unread += biff_->mailbox(i)->unreads();
	}

	// We play sound only if watch was not asked explicitely
	if ((newmail == true) && (unread > 0) && (force_popup_ == false)) {
		if (biff_->sound_type_ == SOUND_BEEP) {
			gdk_beep ();    
		}
		else if (biff_->sound_type_ == SOUND_FILE) {
			std::stringstream s;
			s << biff_->sound_volume_/100.0f;
			std::string command = biff_->sound_command_;
			guint i;
			while ((i = command.find ("%s")) != std::string::npos) {
				command.erase (i, 2);
				std::string filename = std::string("\"") + biff_->sound_file_ + std::string("\"");
				command.insert(i, filename);
			}
			while ((i = command.find ("%v")) != std::string::npos) {
				command.erase (i, 2);
				command.insert(i, s.str());
			}
			command += " &";
			system (command.c_str());
		}
	}

	// Check if we display popup window depending on popup value:
	if (unread && ((biff_->popup_display_ && newmail) || (force_popup_))) {
		biff_->popup()->update();
		biff_->popup()->show();
	}
	else {
		watch_on();
	}

	force_popup_ = false;
	g_mutex_unlock (process_mutex_);
}
