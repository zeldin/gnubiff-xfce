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
// Author(s)     : Robert Sowada, Nicolas Rougier
// Short         : Options for gnubiff
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#include "gnubiff_options.h"
#include "nls.h"

const guint Gnubiff_Options::protocol_int[] = {
	PROTOCOL_NONE, PROTOCOL_FILE, PROTOCOL_POP3, PROTOCOL_IMAP4,
	PROTOCOL_MAILDIR, PROTOCOL_MH, PROTOCOL_APOP, 0
};
const gchar *Gnubiff_Options::protocol_gchar[] = {
	"-", "file", "pop3", "imap4", "maildir", "mh", "apop", NULL
};

/**
 *  Add options of the given groups.
 *
 *  @param  groups  Groups of which the options shall be added.
 */
void 
Gnubiff_Options::add_options (guint groups)
{
	if (groups & OPTGRP_APPLET)
		add_options_applet ();

	if (groups & OPTGRP_GENERAL)
		add_options_general ();

	if (groups & OPTGRP_INFORMATION)
		add_options_information ();

	if (groups & OPTGRP_MAILBOX)
		add_options_mailbox ();

	if (groups & OPTGRP_POPUP)
		add_options_popup ();

	if (groups & OPTGRP_SECURITY)
		add_options_security ();
}

/// Add options for the appearance of the applet
void 
Gnubiff_Options::add_options_applet (void)
{
	add_group (new Option_Group ("applet", OPTGRP_APPLET,
		"Appearance of the applet."));

	// APPLET_USE_DECORATION
	add_option (new Option_Bool ("applet_use_decoration", OPTGRP_APPLET,
		"Shall the gnubiff applet window have window manager decoration?",
								 false, OPTFLG_NONE, OPTGUI_TOGGLE,
								 "applet_decoration_check"));
	// APPLET_BE_STICKY
	add_option (new Option_Bool ("applet_be_sticky", OPTGRP_APPLET,
		"Shall the gnubiff applet window be sticky (i.e. appear on all "
		"workspaces)?",
								 false));
	// APPLET_KEEP_ABOVE
	add_option (new Option_Bool ("applet_keep_above", OPTGRP_APPLET,
		"Shall the gnubiff applet window always be kept above other windows?",
								 false));
	// APPLET_PAGER
	add_option (new Option_Bool ("applet_pager", OPTGRP_APPLET,
		"Shall the gnubiff applet window appear in pagers?",
								 false));
	// USE_NEWMAIL_TEXT
	const static gchar *s1[] = {"newmail_text_entry", NULL};
	add_option (new Option_Bool ("use_newmail_text", OPTGRP_APPLET,
		"Shall a text be printed into the applet if new mails are present?",
								 true, OPTFLG_NONE, OPTGUI_TOGGLE,
								 "newmail_text_check", s1));
	// NEWMAIL_TEXT
	add_option (new Option_String ("newmail_text", OPTGRP_APPLET,
		"Text to be printed into the applet if new mails are present "
        "(%d is the number of mails).",
								   "%d", OPTFLG_NONE, OPTGUI_ENTRY,
								   "newmail_text_entry"));
	// USE_NEWMAIL_IMAGE
	const static gchar *s2[] = {"newmail_image_entry", "newmail_image_browse",
								NULL};
	add_option (new Option_Bool ("use_newmail_image", OPTGRP_APPLET,
		"Shall a image be displayed in the applet if new mails are present?",
								 true, OPTFLG_NONE, OPTGUI_TOGGLE,
								 "newmail_image_check", s2));
	// NEWMAIL_IMAGE
	add_option (new Option_String ("newmail_image", OPTGRP_APPLET,
		"Filename of the image to be displayed in the applet if new mails are "
        "present.",
								   GNUBIFF_DATADIR"/tux-awake.png",
								   OPTFLG_TEST_FILE, OPTGUI_ENTRY,
								   "newmail_image_entry"));
	// USE_NOMAIL_TEXT
	const static gchar *s3[] = {"nomail_text_entry", NULL};
	add_option (new Option_Bool ("use_nomail_text", OPTGRP_APPLET,
		"Shall a text be printed into the applet if no mails are present?",
								 true, OPTFLG_NONE, OPTGUI_TOGGLE,
								 "nomail_text_check", s3));
	// NOMAIL_TEXT
	add_option (new Option_String ("nomail_text", OPTGRP_APPLET,
		"Text to be printed into the applet if no mails are present.",
								   _("no mail"), OPTFLG_NONE, OPTGUI_ENTRY,
								   "nomail_text_entry"));
	// USE_NOMAIL_IMAGE
	const static gchar *s4[] = {"nomail_image_entry", "nomail_image_browse",
								NULL};
	add_option (new Option_Bool ("use_nomail_image", OPTGRP_APPLET,
		"Shall a image be displayed in the applet if no mails are present?",
								 true, OPTFLG_NONE, OPTGUI_TOGGLE,
								 "nomail_image_check", s4));
	// NOMAIL_IMAGE
	add_option (new Option_String ("nomail_image", OPTGRP_APPLET,
		"Filename of the image to be displayed in the applet if no mails are "
        "present.",
								   GNUBIFF_DATADIR"/tux-sleep.png",
								   OPTFLG_TEST_FILE, OPTGUI_ENTRY,
								   "nomail_image_entry"));
	// APPLET_USE_GEOMETRY
	const static gchar *s5[] = {"applet_geometry_entry", NULL};
	add_option (new Option_Bool ("applet_use_geometry", OPTGRP_APPLET,
		"Shall the given geometry be used for positioning the applet window?",
								 true, OPTFLG_NONE, OPTGUI_TOGGLE,
								 "applet_geometry_check", s5));
	// APPLET_GEOMETRY
	add_option (new Option_String ("applet_geometry", OPTGRP_APPLET,
		"Geometry to be used for positioning the applet window.",
								   "+0+0", OPTFLG_NONE, OPTGUI_ENTRY,
								   "applet_geometry_entry"));
	// APPLET_FONT
	add_option (new Option_String ("applet_font", OPTGRP_APPLET,
		"Font to be used in the applet.",
								   "sans 10", OPTFLG_NONE, OPTGUI_FONT,
								   "applet_font_button"));
}

/// Add general options
void 
Gnubiff_Options::add_options_general (void)
{
	add_group (new Option_Group ("general", OPTGRP_GENERAL,
		"General options."));

	// CONFIG_FILE
	gchar *filename = g_build_filename (g_get_home_dir (),".gnubiffrc", NULL);
	add_option (new Option_String ("config_file", OPTGRP_GENERAL,
		"Filename of the configuration file.",
								   filename, OPTFLG_NOSAVE));
	g_free(filename);
	// NEWMAIL_COMMAND_ENTRY
	const static gchar *s2[] = {"newmail_command_entry", NULL};
	add_option (new Option_Bool ("use_newmail_command", OPTGRP_GENERAL,
		"Shall a command be executed if new mail arrives?",
								 true, OPTFLG_NONE,
								 OPTGUI_TOGGLE, "newmail_command_check", s2));
	// NEWMAIL_COMMAND
	add_option (new Option_String ("newmail_command", OPTGRP_GENERAL,
		"Command to be executed if new mail arrives.",
								   "play "GNUBIFF_DATADIR"/coin.wav",
								   OPTFLG_NONE, OPTGUI_ENTRY,
								   "newmail_command_entry"));
	// DOUBLE_COMMAND_ENTRY
	const static gchar *s3[] = {"double_command_entry", NULL};
	add_option (new Option_Bool ("use_double_command", OPTGRP_GENERAL,
		"Shall a command be executed if the gnubiff applet is doubleclicked?",
								 true, OPTFLG_NONE, OPTGUI_TOGGLE,
								 "double_command_check", s3));
	// DOUBLE_COMMAND
	add_option (new Option_String ("double_command", OPTGRP_GENERAL,
		"Command to be executed if the gnubiff applet is doubleclicked.",
								   "xemacs", OPTFLG_NONE, OPTGUI_ENTRY,
								   "double_command_entry"));
	// CHECK_MODE
	const static guint i4[] = {MANUAL_CHECK, AUTOMATIC_CHECK, 0};
	const static gchar *s4[] = {"manual", "automatic", NULL};
	add_option (new Option_UInt ("check_mode", OPTGRP_GENERAL,
		"Automatic or manual checking for new mails?",
								 AUTOMATIC_CHECK,
								 OPTFLG_NOSAVE | OPTFLG_ID_INT_STRICT, i4,s4));
	// MIN_BODY_LINES
	add_option (new Option_UInt ("min_body_lines", OPTGRP_GENERAL,
		"Minimum number of body lines of a mail to be read. If the mail's "
		"body is shorter then the whole body is read. If supported by the "
		"protocol gnubiff tries to read exactly this number of lines.",
								 12));
	// EXPERT_SHOW_TAB
	const static gchar *s5[] = {"expert_vbox", NULL};
	add_option (new Option_Bool ("expert_show_tab", OPTGRP_GENERAL,
		"Shall the expert dialog for editing all options be shown? Note: If "
		"this option is set to \"false\" it can only be changed to \"true\" "
		"by editing the config file manually. The default value of this "
		"option can be changed via an option to configure.",
#ifdef EXPERT_SHOW_NO_TAB
								 false,
#else
								 true,
#endif
								 OPTFLG_NONE, OPTGUI_TOGGLE,
								 "expert_show_tab_check", NULL, s5));
	// EXPERT_EDIT_OPTIONS
	const static gchar *s6[] = {"!expert_warning_vbox", "expert_editing_vbox",
								NULL};
	add_option (new Option_Bool ("expert_edit_options", OPTGRP_GENERAL,
		"Shall expert mode editing be enabled? Otherwise a warning message is "
		"shown inside of the expert tab.",
								 false, OPTFLG_NONE, OPTGUI_TOGGLE,
								 "expert_edit_options_check", NULL, s6));
	// EXPERT_SHOW_FIXED
	add_option (new Option_Bool ("expert_show_fixed", OPTGRP_GENERAL,
		"Shall options be displayed in the expert dialog that cannot be "
		"changed?",
								 true));
#ifdef DEBUG
	// EXPERT_SHOW_NOSHOW
	add_option (new Option_Bool ("expert_show_noshow", OPTGRP_GENERAL,
		"Shall options be displayed in the expert dialog that are flagged "
		"for not to be shown? Usually there should be no need to view these "
		"options as they are only needed for internal use and can only be "
		"changed by setting other options. Viewing them may be of interest "
		"when debugging.",
								 false));
#endif
	// EXPERT_SEARCH_VALUES
	add_option (new Option_Bool ("expert_search_values", OPTGRP_GENERAL,
		"When searching for options that contain a given string, examine "
		"option name and value if this option is true, otherwise examine "
		"only the option name?",
								 false));
	// EXPERT_HILITE_CHANGED
	add_option (new Option_Bool ("expert_hilite_changed", OPTGRP_GENERAL,
		"Shall all options that have not their default values and are "
		"editable by the user be highlighted?",
								 true));
	// PREF_ALLOW_RESIZE
	add_option (new Option_Bool ("pref_allow_resize", OPTGRP_GENERAL,
		"Shall it be allowed to resize the preferences dialog window?",
								 false));
}

/// Add options that are for information purposes only
void 
Gnubiff_Options::add_options_information (void)
{
	add_group (new Option_Group ("information", OPTGRP_INFORMATION,
		"Not to be changed, for information purposes only."));

	// GTK_MODE
	const static gchar *s1[] = {"applet_geometry_check",
								"applet_geometry_entry",
								"applet_decoration_check", NULL};
	add_option (new Option_Bool ("gtk_mode", OPTGRP_INFORMATION,
		"Is gnubiff in GTK mode?",
								 true, OPTFLG_NOSAVE | OPTFLG_AUTO
								 | OPTFLG_NOSHOW, OPTGUI_NONE, "", s1));
	// PROTOCOL
	add_option (new Option_UInt ("protocol", OPTGRP_INFORMATION,
		"For internal use only when loading config file.",
								 PROTOCOL_NONE, OPTFLG_ID_INT_STRICT 
								 |OPTFLG_FIXED | OPTFLG_NOSAVE | OPTFLG_NOSHOW,
								 protocol_int, protocol_gchar));
	// UI_MODE
	const static guint i2[] = {GTK_MODE, GNOME_MODE, 0};
	const static gchar *s2[] = {"gtk", "gnome", NULL};
	add_option (new Option_UInt ("ui_mode", OPTGRP_INFORMATION,
		"User interface mode in which gnubiff is running.",
								 GTK_MODE, OPTFLG_CHANGE | OPTFLG_ID_INT_STRICT
								 | OPTFLG_FIXED | OPTFLG_NOSAVE, i2, s2));
}

/// Add options that are different for each mailbox.
void 
Gnubiff_Options::add_options_mailbox (void)
{
	add_group (new Option_Group ("mailbox", OPTGRP_MAILBOX,
		"Options that are mailbox dependant."));

	// ADDRESS
	std::string address;
	if (g_getenv ("MAIL"))
		address = g_getenv ("MAIL");
	else if (g_getenv ("HOSTNAME"))
		address = g_getenv ("HOSTNAME");
	add_option (new Option_String ("address", OPTGRP_MAILBOX,
		"Address of the mailbox. For local mailboxes this is the name of the "
		"file or directory, for network mailboxes this is the internet "
		"address.",
								   address, OPTFLG_NONE, OPTGUI_ENTRY,
								   "address_entry"));
	// AUTHENTICATION
	const static guint i4[] = {AUTH_AUTODETECT, AUTH_USER_PASS, AUTH_APOP,
							   AUTH_SSL, AUTH_CERTIFICATE, AUTH_NONE, 0};
	const static gchar *s4[] = {"autodetect", "user_pass", "apop", "ssl",
								"certificate", "-", NULL};
	add_option (new Option_UInt ("authentication", OPTGRP_MAILBOX,
		"Authentication to be used when connecting to the server via the "
		"internet.\n"
		"Attention: Don't use autodetection if you don't want your password "
		"to be sent over the network in clear. There may be a "
		"man-in-the-middle attack resulting in gnubiff sending the password "
		"in clear even if your server supports APOP or SSL.",
								 AUTH_AUTODETECT,
								 OPTFLG_CHANGE | OPTFLG_ID_INT_STRICT, i4,s4));
	// CERTIFICATE
	add_option (new Option_String ("certificate", OPTGRP_MAILBOX,
		"Certificate to be used when using SSL.",
								   "", OPTFLG_NONE, OPTGUI_ENTRY,
								   "certificate_entry"));
	// NAME
	add_option (new Option_String ("name", OPTGRP_MAILBOX,
		"Name of the mailbox.",
								   "", OPTFLG_NONE, OPTGUI_ENTRY,
								   "name_entry"));
	// DELAY
	add_option (new Option_UInt ("delay", OPTGRP_MAILBOX,
		"Time interval between mail checks for network mailboxes when "
		"polling (in seconds).",
								 180, OPTFLG_CHANGE));
	// DELAY_MINUTES
	add_option (new Option_UInt ("delay_minutes", OPTGRP_MAILBOX,
		"Minute part of the time interval between mail checks for network "
		"mailboxes when polling.",
								 3, OPTFLG_CHANGE | OPTFLG_NOSAVE,
								 OPTGUI_SPIN, "minutes_spin"));
	// DELAY_SECONDS
	add_option (new Option_UInt ("delay_seconds", OPTGRP_MAILBOX,
		"Second part of the time interval between mail checks for network "
		"mailboxes when polling.",
								 0, OPTFLG_CHANGE | OPTFLG_NOSAVE,
								 OPTGUI_SPIN, "seconds_spin"));
	// USE_OTHER_FOLDER
	const static gchar *s3[] = {"mailbox_entry", NULL};
	add_option (new Option_Bool ("use_other_folder", OPTGRP_MAILBOX,
		"Shall not the standard folder be used when accessing an Imap4 "
		"server?",
								 false, OPTFLG_CHANGE, OPTGUI_RADIO,
								 "standard_mailbox_radio other_mailbox_radio",
								 s3));
	// OTHER_FOLDER
	add_option (new Option_String ("other_folder", OPTGRP_MAILBOX,
		"Folder to be used when accessing a non standard folder on the Imap4 "
		"server.",
								   "INBOX", OPTFLG_CHANGE, OPTGUI_ENTRY,
								   "mailbox_entry"));
	// FOLDER
	add_option (new Option_String ("folder", OPTGRP_MAILBOX,
		"Folder to be used when accessing an Imap4 server.",
								   "INBOX", OPTFLG_AUTO | OPTFLG_NOSAVE));
	// PASSWORD
	add_option (new Option_String ("password", OPTGRP_MAILBOX,
		"Password of the mailbox. This is needed to login into network "
		"mailboxes.",
								   "",
#ifdef USE_PASSWORD
								   OPTFLG_NONE,
#else
								   OPTFLG_NOSAVE,
#endif
								   OPTGUI_ENTRY, "password_entry"));
	// USE_OTHER_PORT
	const static gchar *s2[] = {"port_spin", NULL};
	add_option (new Option_Bool ("use_other_port", OPTGRP_MAILBOX,
		"Shall not the standard port be used when connection to the server "
		"via the internet?",
								 false, OPTFLG_CHANGE, OPTGUI_RADIO,
								 "standard_port_radio other_port_radio", s2));
	// OTHER_PORT
	add_option (new Option_UInt ("other_port", OPTGRP_MAILBOX,
		"Port to be used when connecting to the server via the internet on "
		"a non standard port.",
								   995, OPTFLG_CHANGE, OPTGUI_SPIN,
								   "port_spin"));
	// PORT
	add_option (new Option_UInt ("port", OPTGRP_MAILBOX,
		"Port to be used when connecting to the server via the internet. "
		"If authentication is set to autodetect this option will be set "
		"when the mailbox is connected for the first time.",
								 995, OPTFLG_AUTO | OPTFLG_NOSAVE));
	// PROTOCOL
	add_option (new Option_UInt ("protocol", OPTGRP_MAILBOX,
		"Protocol to be used by the mailbox.",
								 PROTOCOL_NONE, OPTFLG_ID_INT_STRICT 
								 | OPTFLG_FIXED | OPTFLG_CHANGE, protocol_int,
								 protocol_gchar));
	// SEEN
	add_option (new Option_String ("seen", OPTGRP_MAILBOX,
		"Space separated list of mail identifiers of mails that have been "
		"marked as read.",
								   "", OPTFLG_CHANGE | OPTFLG_UPDATE
								   | OPTFLG_STRINGLIST));
	// STATUS
	const static guint i5[] = {MAILBOX_ERROR, MAILBOX_EMPTY, MAILBOX_OLD,
							   MAILBOX_NEW, MAILBOX_CHECK, MAILBOX_STOP,
							   MAILBOX_UNKNOWN, 0};
	const static gchar *s5[] = {"error", "empty", "old", "new", "check",
								"stop", "unknown", NULL};
	add_option (new Option_UInt ("status", OPTGRP_MAILBOX,
		"Status of the mailbox.",
								 MAILBOX_UNKNOWN, OPTFLG_ID_INT_STRICT 
								 | OPTFLG_AUTO | OPTFLG_NOSAVE, i5, s5));
	// UIN
	add_option (new Option_UInt ("uin", OPTGRP_MAILBOX,
		"Unique identifier number of the mailbox.",
								 0, OPTFLG_CHANGE | OPTFLG_FIXED
								 | OPTFLG_NOSAVE));
	// USE_IDLE
	add_option (new Option_Bool ("use_idle", OPTGRP_MAILBOX,
		"Shall the IDLE command be used if the IMAP4 server supports it? "
		"This is usually a good idea. But if multiple clients connect to the "
		"same mailbox this can lead to connection errors (depending on the "
		"internal server configuration). For users encountering this problem "
		"it is better not to use idling but use polling instead.",
								 true));
	// USERNAME
	std::string username;
	if (g_get_user_name ())
		username = g_get_user_name ();
	add_option (new Option_String ("username", OPTGRP_MAILBOX,
		"Username of the mailbox. This is needed to login into network "
		"mailboxes.",
								   username, OPTFLG_NONE, OPTGUI_ENTRY,
								   "username_entry"));
}

/// Add options for the appearance of the popup
void 
Gnubiff_Options::add_options_popup (void)
{
	add_group (new Option_Group ("popup", OPTGRP_POPUP,
		"Appearance of the popup."));

	// USE_POPUP
	const static gchar *s1[] = {"popup_delay_spin", NULL};
	add_option (new Option_Bool ("use_popup", OPTGRP_POPUP,
		"Shall a popup window displayed when new mails are present?",
								 true, OPTFLG_NONE, OPTGUI_TOGGLE,
								 "use_popup_check", s1));
	// POPUP_DELAY
	add_option (new Option_UInt ("popup_delay", OPTGRP_POPUP,
		"Time that the popup window will be shown when new mails are present "
		"(in seconds).",
								 5, OPTFLG_NONE, OPTGUI_SPIN,
								 "popup_delay_spin"));
	// POPUP_USE_DECORATION
	add_option (new Option_Bool ("popup_use_decoration", OPTGRP_POPUP,
		"Shall the gnubiff popup window have window manager decoration?",
								 false, OPTFLG_NONE, OPTGUI_TOGGLE,
								 "popup_decoration_check"));
	// POPUP_BE_STICKY
	add_option (new Option_Bool ("popup_be_sticky", OPTGRP_POPUP,
		"Shall the gnubiff popup window be sticky (i.e. appear on all "
		"workspaces)?",
								 false));
	// POPUP_KEEP_ABOVE
	add_option (new Option_Bool ("popup_keep_above", OPTGRP_POPUP,
		"Shall the gnubiff popup window always be kept above other windows?",
								 false));
	// POPUP_PAGER
	add_option (new Option_Bool ("popup_pager", OPTGRP_POPUP,
		"Shall the gnubiff popup window appear in pagers?",
								 false));
	// POPUP_USE_GEOMETRY
	const static gchar *s5[] = {"popup_geometry_entry", NULL};
	add_option (new Option_Bool ("popup_use_geometry", OPTGRP_POPUP,
		"Shall the given geometry be used for positioning the popup window?",
								 true, OPTFLG_NONE, OPTGUI_TOGGLE,
								 "popup_geometry_check", s5));
	// POPUP_GEOMETRY
	add_option (new Option_String ("popup_geometry", OPTGRP_POPUP,
		"Geometry to be used for positioning the popup window.",
								   "-0+0", OPTFLG_NONE, OPTGUI_ENTRY,
								   "popup_geometry_entry"));
	// POPUP_FONT
	add_option (new Option_String ("popup_font", OPTGRP_POPUP,
		"Font to be used in the popup.",
								   "sans 10", OPTFLG_NONE, OPTGUI_FONT,
								   "popup_font_button"));
	// POPUP_USE_SIZE
	const static gchar *s6[] = {"popup_size_spin", NULL};
	add_option (new Option_Bool ("popup_use_size", OPTGRP_POPUP,
		"Shall there be a restriction to the number of mails displayed in "
		"the popup?",
								 true, OPTFLG_NONE, OPTGUI_TOGGLE,
								 "popup_size_check", s6));
	// POPUP_SIZE
	add_option (new Option_UInt ("popup_size", OPTGRP_POPUP,
		"Maximum number of mails to be displayed in the popup.",
								   40, OPTFLG_NONE, OPTGUI_SPIN,
								   "popup_size_spin"));
	// POPUP_USE_FORMAT
	const static gchar *s7[] = {"popup_format_entry", NULL};
	add_option (new Option_Bool ("popup_use_format", OPTGRP_POPUP,
		"Shall there be a restriction to the length of the sender, subject "
		"and date when displayed in the popup?",
								 true, OPTFLG_NONE, OPTGUI_TOGGLE,
								 "popup_format_check", s7));
	// POPUP_SIZE_SENDER
	add_option (new Option_UInt ("popup_size_sender", OPTGRP_POPUP,
		"Maximum size of the sender when displayed in the popup (in "
		"characters).",
								 50, OPTFLG_NOSAVE | OPTFLG_CHANGE));
	// POPUP_SIZE_SUBJECT
	add_option (new Option_UInt ("popup_size_subject", OPTGRP_POPUP,
		"Maximum size of the subject when displayed in the popup (in "
		"characters).",
								 50, OPTFLG_NOSAVE | OPTFLG_CHANGE));
	// POPUP_SIZE_DATE
	add_option (new Option_UInt ("popup_size_date", OPTGRP_POPUP,
		"Maximum size of the date when displayed in the popup (in "
		"characters).",
								 50, OPTFLG_NOSAVE | OPTFLG_CHANGE));
	// POPUP_FORMAT
	add_option (new Option_String ("popup_format", OPTGRP_POPUP,
		"Length restrictions to the length of the sender, subject and date "
		"when displayed in the popup. This has to be given as a colon "
		"separated list. A value of 0 disables the display of the "
		"corresponding property.",
								   "50:50:50", OPTFLG_CHANGE, OPTGUI_ENTRY,
								   "popup_format_entry"));
	// POPUP_BODY_LINES
	add_option (new Option_UInt ("popup_body_lines", OPTGRP_POPUP,
		"Maximum number of mail body lines that will be displayed in the "
		"popup.",
								 10));
	// POPUP_SORT_BY
	add_option (new Option_String ("popup_sort_by", OPTGRP_POPUP,
		"A space separated list of header properties by which the headers "
		"in the popup shall be sorted. The headers are sorted first by the "
		"first given property then by the second and so on. A stable sort "
		"algorithm is being used. Currently the following properties are "
		"supported:\n"
		"   * \"date\": Date when the mail was sent\n"
		"   * \"mailbox\": Mailbox identifier. If mails from each mailbox "
        "shall stay together this should be the last given property.\n"
		"   * \"position\": Position of the mail in the mailbox\n"
		"   * \"sender\": Sender of the mail\n"
		"   * \"subject\": Subject of the mail\n"
		"The sorting order of each property can be reversed by prefixing a "
		"\"!\".",
								   "!position mailbox"));
}

/// Add options that affect security issues.
void 
Gnubiff_Options::add_options_security (void)
{
	add_group (new Option_Group ("security", OPTGRP_SECURITY,
		"Options that affect security issues. Most of these options help "
        "gnubiff in deciding whether it is DoS attacked or not."));

	// USE_MAX_MAIL
	const static gchar *s1[] = {"max_mail_spin", NULL};
	add_option (new Option_Bool ("use_max_mail", OPTGRP_SECURITY,
		"Shall there be any restriction to the number of mails that are "
		"collected?",
								 true, OPTFLG_NONE, OPTGUI_TOGGLE,
								 "max_mail_check", s1));
	// MAX_MAIL
	add_option (new Option_UInt ("max_mail", OPTGRP_SECURITY,
		"The maximum number of mails that will be collected per update and "
		"mailbox.",
								 100, OPTFLG_NONE, OPTGUI_SPIN,
								 "max_mail_spin"));
	// PREVDOS_ADDITIONAL_LINES
	add_option (new Option_UInt ("prevdos_additional_lines", OPTGRP_SECURITY,
		"Maximum number of lines that are read from the network additionally "
		"to the number of lines that are expected when reading until a "
		"certain line is sent by the server. There are many possible "
		"reasons, why the number of lines that are sent is greater than "
        "expected:\n"
		"   * The server sends information or warning messages (IMAP4 for "
		"example; see RFC 3501 7.1.1 and 7.1.2)\n"
		"   * There exist extensions to the protocols\n"
		"   * The server may implement a protocol not correctly\n"
		"   * There is a DoS attack\n"
		"This option is currently used for the IMAP4 protocol.",
								 16));
	// PREVDOS_HEADER_LINES
	add_option (new Option_UInt ("prevdos_header_lines", OPTGRP_SECURITY,
		"Maximum number of mail header lines that are read.\n"
		"This option is currently used for the POP3 protocol.",
								 2048));
	// PREVDOS_IGNORE_INFO
	add_option (new Option_UInt ("prevdos_ignore_info", OPTGRP_SECURITY,
		"Maximum number of lines that are read from the network when the "
		"server is expected to need a lot of time to complete a command (the "
		"IMAP4 \"IDLE\" command for example) but may send information and "
		"warning messages before completion.\n"
		"This option is currently used for the IMAP4 protocol.",
								 32));
	// PREVDOS_IMAP4_MULTILINE
	add_option (new Option_UInt ("prevdos_imap4_multiline", OPTGRP_SECURITY,
		"Maximum number of lines that are read additional from the network "
		"when reading the server's response to IMAP4 commands that consist "
		"of more than one line."
		"See also the description of the "
		"\"security/prevdos_additional_lines\" option.\n"
		"This option is only intended for the IMAP4 protocol.",
								 8));
	// PREVDOS_LINE_LENGTH
	add_option (new Option_UInt ("prevdos_line_length", OPTGRP_SECURITY,
		"Maximum number of characters per line in mails. The following "
		"limits are set for the different protocols:\n"
		"   * SMTP: maximum line length is 1001 (see RFC 2821 4.5.3.1)\n"
		"   * IMAP4: no maximum line length\n"
		"   * POP3: maximum response line length is 512 (see RFC 1939 3.)\n"
		"This option is currently used for all network protocols.",
								 16384));
	// PREVDOS_CLOSE_SOCKET
	add_option (new Option_UInt ("prevdos_close_socket", OPTGRP_SECURITY,
		"Maximum number of lines to be read when the socket for a network "
		"connection is closed.\n"
		"This option is used for all network protocols.",
								 64));
}
