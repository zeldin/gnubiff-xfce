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
#ifndef _BIFF_H
#define _BIFF_H

#ifdef HAVE_CONFIG_H
#   include "../config.h"
#endif
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <glib.h>
#include "Setup.h"
#include "Applet.h"

#ifdef ENABLE_NLS
#  include<libintl.h>
#  ifndef USE_GNOME
#    define _(String) dgettext(GETTEXT_PACKAGE,String)
#  else
#    include <gnome.h>
#  endif
#  ifdef gettext_noop
#    define N_(String) gettext_noop(String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define _(String) (String)
#  define N_(String) (String)
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,String) (String)
#  define dcgettext(Domain,String,Type) (String)
#  define bindtextdomain(Domain,Directory) (Domain)
#endif


// ===================================================================
// - Constant definitions --------------------------------------------
// ===================================================================
#define GTK_MODE				0
#define GNOME_MODE				1

#define PROTOCOL_FILE			0
#define PROTOCOL_POP3			1
#define PROTOCOL_IMAP4			2
#define PROTOCOL_MAILDIR		3
#define PROTOCOL_MH				4
#define PROTOCOL_APOP			5
#define DEFAULT_POP3_PORT		110
#define DEFAULT_IMAP4_PORT		143
#define DEFAULT_SSL_POP3_PORT	995
#define DEFAULT_SSL_IMAP4_PORT	993

#define DEFAULT_IMAP4_FOLDER	"INBOX"
#define DEFAULT_MAILDIR_FOLDER	"new"

#define	SOUND_NONE				0
#define SOUND_BEEP				1
#define SOUND_FILE				2

#define SIGN_PLUS				TRUE
#define SIGN_MINUS				FALSE

#define STATE_RUNNING			TRUE
#define STATE_STOPPED			FALSE


// ===================================================================
// - Type definitions ------------------------------------------------
// ===================================================================
typedef struct _header {
	std::string	sender;
	std::string	subject;
	std::string	date;
	std::string	body;
	std::string	charset;
	gint	status;
	// Copy operator
	struct _header &operator = (const struct _header &other) {
		if (this == &other) return *this;
		sender = other.sender;
		subject = other.subject;
		date = other.date;
		body = other.body;
		charset = other.charset;
		status = other.status;
		return *this;
	}
	// Comparison operator
	bool operator == (const struct _header &other) const {
		if ((sender  == other.sender)  && (subject == other.subject) &&
			(date    == other.date)    && (body    == other.body)    &&
			(charset == other.charset) && (status  == other.status))
			return true;
		else
			return false;
	}
} header;


class Biff {
	// ===================================================================
	// - Public methods --------------------------------------------------
	// ===================================================================
 public:
	Biff (gint mode=GTK_MODE, std::string filename = "");
	~Biff (void);
	gboolean load (void);
	gboolean save (void);

	void xml_start_element (GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, GError **error);
	void xml_error (GMarkupParseContext *context, GError *error);

	class Setup *setup (void)						{return _setup;}
	class Applet *applet (void)						{return _applet;}
	class Popup *popup (void)						{return _popup;}
	class Mailbox *&mailbox (int i)					{return _mailbox[i];}
	std::vector<class Mailbox *> &mailbox (void)	{return _mailbox;}
	guint mailboxes (void)							{return _mailbox.size();}


	// ===================================================================
	// - Public attributes -----------------------------------------------
	// ===================================================================
 public:
	guint			_mode;					// GTK or GNOME mode

	guint			_poptime;				// Time (seconds) to display popup
	guint			_sound_type;			// Type of sound to be played when new mail
	std::string		_sound_command;			// Command to use to play sound
	std::string		_sound_file;			// Sound file to be used
	guint			_sound_volume;			// Sound volume (0..100)
	gboolean		_state;					// State of gnubiff (stopped or running)
	guint			_max_collected_mail;	// Maximum collected headers

	gboolean		_display_popup;			// Display popup (or not) when new mail
	std::string		_mail_app;				// Filename of the mail client

	std::string		_mail_image;			// Filename of the image or animation for new mail
	std::string		_nomail_image;			// Filename of the image or animation for no new mail
	gboolean		_hide_newmail_image;	// Hide (or not) biff or applet image when new mail
	gboolean		_hide_nomail_image;		// Hide (or not) biff or applet image when no new mail
	gboolean		_hide_newmail_text;		// Hide (or not) biff or applet text when new mail
	gboolean		_hide_nomail_text;		// Hide (or not) biff or applet text when no new mail
	std::string		_newmail_text;			// Text when new mail
	std::string		_nomail_text;			// Text when no new mail
	gboolean		_display_date;			// Display date (or not) in popup
	guint			_max_sender_size;		// Maximum size (characters) of sender field in popup
	guint			_max_subject_size;		// Maximum size (characters) of subject in popup

	gboolean		_popup_x_sign;			// Sign of x position of popup window
	guint			_popup_x;				// X position of popup window
	gboolean		_popup_y_sign;			// Sign of y position of popup window
	guint			_popup_y;				// Y position of popup window
	gboolean		_popup_decorated;		// Decorations for popup window (or not)
	gboolean		_popup_no_geometry;		// Use geometry (or not)
	gboolean		_biff_x_sign;			// Sign of x position of biff window
	guint			_biff_x;				// X position of biff window
	gboolean		_biff_y_sign;			// Sign of y position of biff window
	guint			_biff_y;				// Y position of biff window
	gboolean		_biff_decorated;		// Decorations for biff window (or not)
	gboolean		_biff_no_geometry;		// Use geometry (or not)
	guint			_max_header_display;	// Maximum number of headers to be collected and displayed

	std::string		_popup_font;			// Popup font
	std::string		_applet_font;			// Applet font
	std::string		_popup_font_color;		// Popup font color
	std::string		_popup_back_color;		// Popup back color
	std::string		_applet_font_color;		// Applet font color
	

	// ===================================================================
	// - Private attributes ----------------------------------------------
	// ===================================================================
 private:
	std::string						_table;		// Table used for crypting password
	std::string						_filename;	// Configuration filename
	class Setup *					_setup;		// Setup window (GUI)
	class Applet *					_applet;	// Applet (GUI)
	class Popup *					_popup;		// Popup window (GUI)
	std::vector<class Mailbox *>	_mailbox;	// Mailbox
	int								_counter;	// Counter used to load mailboxes information
};

#endif
