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
#include <iomanip>
#include <cstdio>
#include <string>
#include <glib.h>
#include "Applet.h"
#include "Mailbox.h"
#include "Popup.h"


// ================================================================================
//  Constructors & Destructors
// --------------------------------------------------------------------------------
//  
// ================================================================================
Applet::Applet (Biff *owner, std::string xmlFilename) : GUI (xmlFilename)
{
	_owner = owner;
	_force_popup = false;
	_process_mutex = g_mutex_new ();
	_update_mutex = g_mutex_new ();
}

Applet::~Applet (void)
{
	g_mutex_lock (_process_mutex);
	g_mutex_unlock (_process_mutex);
	g_mutex_free (_process_mutex);
	g_mutex_lock (_update_mutex);
	g_mutex_unlock (_update_mutex);
	g_mutex_free (_update_mutex);
}


// ================================================================================
//  Watch functions
// --------------------------------------------------------------------------------
//  Those function handles automatic and forced watch
// ================================================================================
void Applet::watch (void)
{
	_force_popup = true;
	for (unsigned int i=0; i<_owner->mailboxes(); i++)
		_owner->mailbox(i)->watch();
}

void Applet::watch_now (void)
{
#ifdef DEBUG
	g_message ("Starting watching mailboxes");
#endif
	_force_popup = false;
	for (unsigned int i=0; i<_owner->mailboxes(); i++) {
#ifdef DEBUG
		g_message ("mailbox %d: watch() function called", i+1);
#endif
		_owner->mailbox(i)->watch();
	}
}

void Applet::watch_on (void)
{
#ifdef DEBUG
	g_message ("Watch on");
#endif
	for (unsigned int i=0; i<_owner->mailboxes(); i++)
		_owner->mailbox(i)->watch_on();
}

void Applet::watch_off (void)
{
#ifdef DEBUG
	g_message ("Watch off");
#endif
	for (unsigned int i=0; i<_owner->mailboxes(); i++)
		_owner->mailbox(i)->watch_off();
}


// ================================================================================
//  Build the unread markup string
// --------------------------------------------------------------------------------
//  
// ================================================================================
guint Applet::unread_markup (std::string &text) {
	// Get max collected mail number in a stringstream
	//  just to have a default string size.
	std::stringstream smax;
	smax << _owner->_max_collected_mail;

	// Get number of unread mails
	guint unread = 0;
	for (unsigned int i=0; i<_owner->mailboxes(); i++)
		unread += _owner->mailbox(i)->unreads();
	std::stringstream unreads;
	unreads << std::setfill('0') << std::setw (smax.str().size()) << unread;

	// Applet label (number of mail)

	text = "<span font_desc=\"";
	text += _owner->_applet_font;
	text += "\">";
	text += "<span color=\"";
	text += _owner->_applet_font_color;
	text += "\"><b>";

	std::string ctext;
	if (unread == 0) {
		ctext = _owner->_nomail_text;
		guint i;
		while ((i = ctext.find ("%d")) != std::string::npos) {
			ctext.erase (i, 2);
			ctext.insert(i, unreads.str());
		}
	}
	else if (unread < _owner->_max_collected_mail) {
		ctext = _owner->_newmail_text;
		guint i;
		while ((i = ctext.find ("%d")) != std::string::npos) {
			ctext.erase (i, 2);
			ctext.insert(i, unreads.str());
		}
	}
	else {
		ctext = _owner->_newmail_text;
		guint i;
		while ((i = ctext.find ("%d")) != std::string::npos) {
			ctext.erase (i, 2);
			ctext.insert(i, std::string(smax.str().size(), '+'));
		}
	}
	text += ctext;
	text += "</b></span></span>";
	
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
	smax << _owner->_max_collected_mail;

	std::string tooltip;
	for (unsigned int i=0; i<_owner->mailboxes(); i++) {
		tooltip += _owner->mailbox(i)->name();
		tooltip += " : ";
		std::stringstream s;
		s << std::setfill('0') << std::setw (smax.str().size()) << _owner->mailbox(i)->unreads();

		if (_owner->mailbox(i)->status() == MAILBOX_ERROR)
			tooltip += _("error !");
		else if (_owner->mailbox(i)->status() == MAILBOX_CHECKING) {
			tooltip += "(";
			tooltip += s.str();
			tooltip += ")";
			tooltip += _(" checking...");
		}
		else if (_owner->mailbox(i)->unreads() >= _owner->_max_collected_mail) {
			tooltip += std::string(smax.str().size(), '+');
		}
		else
			tooltip += s.str();
		if (i< (_owner->mailboxes()-1))
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
	if (!g_mutex_trylock (_process_mutex))
		return;
  
	// Look for status of mailboxes
	gboolean newmail = false;
	int unread = 0;
	for (unsigned int i=0; i<_owner->mailboxes(); i++) {
		if (_owner->mailbox(i)->status() == MAILBOX_NEW)
			newmail = true;
		unread += _owner->mailbox(i)->unreads();
	}

	// We play sound only if watch was not asked explicitely
	if ((newmail == true) && (unread > 0) && (_force_popup == false)) {
		if (_owner->_sound_type == SOUND_BEEP) {
			gdk_beep ();    
		}
		else if (_owner->_sound_type == SOUND_FILE) {
			std::stringstream s;
			s << _owner->_sound_volume/100.0f;
			std::string command = _owner->_sound_command;
			guint i;
			while ((i = command.find ("%s")) != std::string::npos) {
				command.erase (i, 2);
				std::string filename = std::string("\"") + _owner->_sound_file + std::string("\"");
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
	if (unread && ((_owner->_display_popup && newmail) || (_force_popup))) {
		_owner->popup()->update();
		_owner->popup()->show();
	}
	else {
		watch_on();
	}

	_force_popup = false;
	g_mutex_unlock (_process_mutex);
}
