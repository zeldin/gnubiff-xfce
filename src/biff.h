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

#ifndef __BIFF_H__
#define __BIFF_H__

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif
#include <string>
#include <vector>
#include <glib.h>

/**
 * Constant definitions
 **/
const guint	GTK_MODE		=	0;
const guint	GNOME_MODE		=	1;
const guint	SOUND_NONE		=	0;
const guint	SOUND_BEEP		=	1;
const guint	SOUND_FILE		=	2;
const guint	MANUAL_CHECK	=	0;
const guint	AUTOMATIC_CHECK	=	1;

/**
 * Type definitions
 **/
typedef struct _header {
	std::string	sender;
	std::string	subject;
	std::string	date;
	std::string	body;
	std::string	charset;
	gint		status;

	struct _header &operator = (const struct _header &other)
	{
		if (this == &other) return *this;
		sender = other.sender;
		subject = other.subject;
		date = other.date;
		body = other.body;
		charset = other.charset;
		status = other.status;
		return *this;
	}

	bool operator == (const struct _header &other) const
	{
		if ((sender  == other.sender)  && (subject == other.subject) &&
			(date    == other.date)    && (body    == other.body)    &&
			(charset == other.charset) && (status  == other.status))
			return true;
		else
			return false;
	}
} header;

#define BIFF(x)		((Biff *)(x))


class Biff {

public:
	/* general */
	guint			ui_mode_;					// GTK or GNOME mode
	gboolean		no_clear_password_;			// Wheter to allow gnubiff to send passwords in clear or not
	guint			sound_type_;				// Type of sound to be played when new mail
	std::string		sound_command_;				// Command to use to play sound
	std::string		sound_file_;				// Sound file to be used
	guint			sound_volume_;				// sound volume (0..100)
	guint			check_mode_;				// gnubiff check mode
	guint			max_mail_;					// Maximum collected email
	std::string		mail_app_;					// mail client

	/* popup */
	gboolean		popup_display_;				// popup display
	guint			popup_time_;				// popup display time
	gboolean		popup_use_geometry_;		// popup use geometry ?
	std::string		popup_geometry_;			// popup geometry (placement)
	gboolean		popup_is_decorated_;		// is popup decorated ?
	guint			popup_max_line_;			// popup max line
	guint			popup_max_sender_size_;		// popup maximum size (characters) of sender field
	guint			popup_max_subject_size_;	// popup maximum size (characters) of subject field
	gboolean		popup_display_date_;		// popup date display
	std::string		popup_font_;				// popup font
	std::string		popup_font_color_;			// popup font color
	std::string		popup_back_color_;			// popup back color

	/* biff */
	std::string		biff_newmail_image_;		// biff image filename for new mail
	std::string		biff_nomail_image_;			// biff image filename for no  mail
	gboolean		biff_use_newmail_image_;	// biff hide image when new mail
	gboolean		biff_use_nomail_image_;		// biff hide image when no  mail
	gboolean		biff_use_newmail_text_;		// biff hide text when new mail
	gboolean		biff_use_nomail_text_;		// biff hide text when no  mail
	std::string		biff_newmail_text_;			// biff text when new mail
	std::string		biff_nomail_text_;			// biff text when no new mail
	gboolean		biff_use_geometry_;			// biff use geometry ?
	std::string		biff_geometry_;				// biff geometry (placement)
	gboolean		biff_is_decorated_;			// is biff decorated ?
	std::string		biff_font_;					// biff font
	std::string		biff_font_color_;			// biff font color


protected:
	/* internal */
	std::string						filename_;		// configuration file
	std::string						passtable_;		// encryption table
	std::vector<class Mailbox *>	mailbox_;		// mailboxes
	gint							count_;			// counter
	GMutex *						lookup_mutex_;	// Mutex for lookup thread
	class Preferences	*			preferences_;	// preferences ui
	class Popup *					popup_;			// popup ui
	class Applet *					applet_;		// applet ui


public:
	/* base */
	Biff (gint ui_mode=GTK_MODE,
		  std::string filename = "");
	~Biff (void);

	/* access */
	guint size (void)						{return mailbox_.size();}
	class Mailbox * mailbox (guint index);
	class Mailbox * find (guint uin);
	class Preferences *preferences (void)	{return preferences_;}
	class Applet *applet (void)				{return applet_;}
	class Popup *popup (void)				{return popup_;}

	/* main */
	void remove (guint uin);						// remove mailbox identified by its uin
	void add (Mailbox *mailbox);					// add a new mailbox
	void lookup (void);								// lookup format of all mailboxes
	void lookup (Mailbox *mailbox);					// lookup format of a specific mailbox
	void lookup (guint index);						// lookup format of a specific mailbox
	void lookup_thread (void);						// lookup thread

	/* I/O */
	gboolean load (void);
	gboolean save (void);
	void xml_start_element (GMarkupParseContext *context,
							const gchar *element_name,
							const gchar **attribute_names,
							const gchar **attribute_values,
							GError **error);
	void xml_error (GMarkupParseContext *context,
					GError *error);
};

#endif
