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

#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <map>
#include <unistd.h>

#ifdef USE_GNOME
#  include "ui-applet-gnome.h"
#endif

#include "biff.h"
#include "ui-applet-gtk.h"
#include "mailbox.h"
#include "file.h"
#include "maildir.h"
#include "mh.h"
#include "mh_basic.h"
#include "mh_sylpheed.h"
#include "imap4.h"
#include "pop3.h"
#include "apop.h"


// ============================================================================
//  "C" binding
// ============================================================================
extern "C" {
	void BIFF_xml_start_element (GMarkupParseContext *context,
								 const gchar *element_name,
								 const gchar **attribute_names,
								 const gchar **attribute_values,
								 gpointer data,
								 GError **error)
	{
		if (data)
			BIFF(data)->xml_start_element (context, element_name,
										   attribute_names, attribute_values,
										   error);
		else
			unknown_internal_error ();
	}
	void BIFF_xml_end_element (GMarkupParseContext *context,
							   const gchar *element_name,
							   gpointer data,
							   GError **error)
	{
		if (data)
			BIFF(data)->xml_end_element (context, element_name, error);
		else
			unknown_internal_error ();
	}

	void BIFF_xml_error (GMarkupParseContext *context,
						 GError *error,
						 gpointer data)
	{
		if (data)
			BIFF(data)->xml_error (context, error);
		else
			unknown_internal_error ();
	}
}

// ============================================================================
//  base
// ============================================================================
Biff::Biff (guint ui_mode, std::string filename)
{
	// Get password table from configure option
#ifdef USE_PASSWORD
	passtable_ = PASSWORD_STRING;
	// Add something (in case the provided string is not long enough)
	passtable_ += "FEDCBA9876543210";
	//  and then we remove duplicated characters
	std::string buffer;
	for (std::string::size_type i = 0; i < passtable_.size(); i++)
		if (buffer.find(passtable_[i]) == std::string::npos)
			buffer += passtable_[i];
	passtable_ = buffer;
#endif

	mutex_ = g_mutex_new ();

	// Authentication mutex
	auth_mutex_ = g_mutex_new ();

	// Add options
	add_options (OPTGRP_ALL & (~OPTGRP_MAILBOX));

	// Set session specific options
	if (filename.size() > 0)
		value ("config_file", filename);
	value ("ui_mode", ui_mode);

	// Does the configuration file exist?
	std::ifstream file;
	file.open (value_gchar ("config_file"));
	if (file.is_open ()) {
		file.close ();
		load ();
	}
	else {
		g_warning (_("Configuration file (%s) not found!"),
				   value_gchar ("config_file"));
		mailbox_.push_back (new Mailbox (this));
	}

	// Applet
	switch (ui_mode) {
	case GTK_MODE:
		applet_ = new AppletGtk (this);
		break;
#ifdef USE_GNOME
	case GNOME_MODE:
		applet_ = new AppletGnome (this);
		break;
#endif
	default:
		applet_ = new AppletGtk (this);
		break;
	}
}

/// Destructor
Biff::~Biff (void)
{
}

// ============================================================================
//  access
// ============================================================================

/**
 *  Search in all mailboxes for the message with id {\em mailid}.
 *
 *  @param  mailid  Gnubiff message identifier of the mail to find
 *  @param  mail    Here the header of the found mail is returned. If no mail
 *                  with id {\em mailid} exists, {\em mail} remains unchanged.
 *  @returns        Boolean indicating if a mail exists or not.
 */
gboolean 
Biff::find_message (std::string mailid, Header &mail)
{
	gboolean ok = false;

	g_mutex_lock (mutex_);
	for (guint i = 0; (i < mailbox_.size()) && !ok; i++)
		if (mailbox_[i]->find_mail (mailid, mail))
			ok = true;
	g_mutex_unlock (mutex_);

	return ok;
}

Mailbox *
Biff::mailbox (guint index)
{
	if (index < mailbox_.size())
		return mailbox_[index];
	return 0;
}

Mailbox *
Biff::get (guint uin)
{
	Mailbox *find = 0;
	g_mutex_lock (mutex_);
	for (guint i = 0; i < mailbox_.size(); i++)
		if (mailbox_[i]->uin() == uin) {
			find = mailbox_[i];
			break;
		}
	g_mutex_unlock (mutex_);
	return find;
}

// ============================================================================
//  main -- mailbox handling
// ============================================================================

/**
 *  Add the new mailbox {\em mailbox} for monitoring.
 *
 *  @param mailbox Mailbox to be added for monitoring.
 */
void 
Biff::add_mailbox (Mailbox *mailbox)
{
	g_mutex_lock (mutex_);
	mailbox_.push_back (mailbox);
	g_mutex_unlock (mutex_);
}

/**
 *  Get the number of mailboxes that are being monitored.
 *
 *  @return   Number of mailboxes.
 */
guint 
Biff::get_number_of_mailboxes (void)
{
	g_mutex_lock (mutex_);
	guint size = mailbox_.size();
	g_mutex_unlock (mutex_);
	return size;
}

/**
 *  Get message headers from the messages in the mailboxes. If
 *  {\em use_max_num} is true, the number of headers is limited to
 *  {\em max_num}. If there are more message headers available than are to be
 *  returned, from each mailbox the same number of headers is taken as far as
 *  possible.
 *
 *  @param  use_max_num  If true only a limited amount of headers is returned
 *                       (the default is false).
 *  @param  max_num      If {\em use_max_num} is true, this is the maximum
 *                       number of message headers to be returned (the default
 *                       is 0).
 *  @return              Vector of message headers
 */
std::vector<Header *> 
Biff::get_message_headers (gboolean use_max_num, guint max_num)
{
	g_mutex_lock (mutex_);

	// Get number of messages in each mailbox
	guint num_mails = 0, num_boxes = 0;
	num_boxes = mailbox_.size();
	std::vector<guint> max (num_boxes);
	for (guint i = 0; i < num_boxes; i++)
		num_mails+= max[i] = mailbox_[i]->unreads();

	// Distribute number of message headers
	std::vector<guint> count (num_boxes, 0);
	if (use_max_num) {
		guint index = 0, all = 0;
		num_mails = std::min (num_mails, max_num);
		while (all < num_mails) {
			if (count[index] < max[index]) {
				count[index]++;
				all++;
			}
			index = (index + 1) % num_boxes;
		}
	}
	else
		count = max;

	// Put all the headers to be displayed in one vector.
	// Note: In the mean time some headers may have been deleted, so less than
	// count[j] headers could be added to the vector
	std::vector<Header *> headers;
	for (guint j = 0; j < num_boxes; j++)
		mailbox_[j]->get_message_headers (headers, true, count[j]);

	g_mutex_unlock (mutex_);

	return headers;
}

/**
 *  Get the number of unread messages in all mailboxes.
 *
 *  @param  num  Here the number of unread messages is returned.
 *  @return      This boolean indicates whether there are new messages (true)
 *               or not (false).
 */
gboolean 
Biff::get_number_of_unread_messages (guint &num)
{
	std::vector<class Mailbox *>::iterator mailbox;
	gboolean newmail = false;
	num = 0;

	g_mutex_lock (mutex_);
	mailbox = mailbox_.begin ();
	while (mailbox != mailbox_.end ()) {
		if ((*mailbox)->status () == MAILBOX_NEW)
			newmail = true;
		num += (*mailbox)->unreads ();
		mailbox++;
	}
	g_mutex_unlock (mutex_);

	return newmail;
}

/**
 * Determine if a password for the given mailbox {\em m} exists. If no
 * password exists yet this function tries to obtain it by:
 * \begin{itemize}
 *    \item Looking at the other mailboxes. If one exists with the same
 *          address, username and port (and a password) it is assumed that
 *          this password is okay.
 *    \item Asking the user. If no other mailbox with the correct parameters is
 *          found the user has to enter the password in a dialog.
 * \end{\itemize}
 *
 * @param  m        the mailbox we want a password for
 * @return          Boolean indicating whether a password could be obtained
 */
gboolean 
Biff::get_password_for_mailbox (Mailbox *m)
{
	// Do we know the password already?
	if (!m->password().empty())
		return true;

	// Remark: It's important to block thread before looking at other mailboxes
	//         since one is maybe asking (using gui) for this password.
	g_mutex_lock (auth_mutex_);

	// Searching other mailboxes
#if DEBUG
	g_message ("[%d] Looking for password for %s@%s:%d", m->uin(),
			   m->username().c_str(), m->address().c_str(), m->port());
#endif

	for (guint i = 0; i < get_number_of_mailboxes (); i++)
		if ((mailbox(i) != m) 
			&& (mailbox(i)->address() == m->address())
			&& (mailbox(i)->username() == m->username())
			&& (mailbox(i)->port() == m->port())
			&& (!mailbox(i)->password().empty())) {
			m->password(mailbox(i)->password());
			break;
		}

	// Ask the user if password is still not known
	if (m->password().empty()) {
		gdk_threads_enter ();
		applet_->get_password_for_mailbox (m);
		gdk_threads_leave ();
	}

	g_mutex_unlock (auth_mutex_);
	return !m->password().empty();
}

/**
 *  Mark all (obtained) messages from all mailboxes as read.
 */
void 
Biff::mark_messages_as_read (void)
{
	std::vector<class Mailbox *>::iterator mailbox;

	g_mutex_lock (mutex_);
	mailbox = mailbox_.begin ();
	while (mailbox != mailbox_.end ())
	  (*(mailbox++))->mark_messages_as_read ();
	g_mutex_unlock (mutex_);
}

/**
 *  This function has to been called, when all (obtained) messages have been
 *  displayed to the user. The status of the mailboxes will be updated.
 */
void 
Biff::messages_displayed (void)
{
	std::vector<class Mailbox *>::iterator mailbox;

	g_mutex_lock (mutex_);
	mailbox = mailbox_.begin ();
	while (mailbox != mailbox_.end ())
	  (*(mailbox++))->mail_displayed ();
	g_mutex_unlock (mutex_);
}

/**
 *  Replace the mailbox {\em from} in the list of all mailboxes by the mailbox
 *  {\em to}. The mailbox {\em from} will be destroyed.
 *
 *  @param  from  Mailbox to be replaced.
 *  @param  to    Mailbox that replaces the mailbox {\em from}.
 *  @return       NULL, if the mailbox {\em from} doesn't exist, otherwise a
 *                pointer to the mailbox {\em to}.
 */
Mailbox *
Biff::replace_mailbox (Mailbox *from, Mailbox *to)
{
	Mailbox *inserted = NULL;

	g_mutex_lock (mutex_);
	for (std::vector<Mailbox *>::iterator i = mailbox_.begin();
		 i != mailbox_.end(); i++)
		if ((*i) == from) {
			(*i) = to;

			// Let the applet do necessary things before the mailbox can be
			// replaced
			applet_->mailbox_to_be_replaced (from, to);

			delete from;
			inserted = to;
			break;
		}
	g_mutex_unlock (mutex_);

	// Start monitoring the new mailbox
	if ((inserted) && (applet_->can_monitor_mailboxes ()))
		inserted->threaded_start (3);
	return inserted;
}

/**
 *  Remove the mailbox {\em mailbox}.
 *
 *  @param mailbox Mailbox to be removed.
 */
void 
Biff::remove_mailbox (Mailbox *mailbox)
{
	g_mutex_lock (mutex_);
	for(std::vector<Mailbox *>::iterator i = mailbox_.begin();
		i != mailbox_.end(); i++)
		if ((*i) == mailbox) {
			mailbox_.erase (i);
			break;
		}
	g_mutex_unlock (mutex_);
}

/**
 *  Start monitoring all mailboxes. Optionally the delay {\em delay} can be
 *  given, so monitoring starts later.
 *
 *  @param  delay  Delay in seconds (the default is 0).
 */
void 
Biff::start_monitoring (guint delay)
{
#ifdef DEBUG
	if (delay)
		g_message ("Start monitoring mailboxes in %d second(s)", delay);
	else
		g_message ("Start monitoring mailboxes now");
#endif
	std::vector<class Mailbox *>::iterator mailbox;

	g_mutex_lock (mutex_);
	mailbox = mailbox_.begin ();
	while (mailbox != mailbox_.end ())
	  (*(mailbox++))->threaded_start (delay);
	g_mutex_unlock (mutex_);
}

/**
 *  Stop monitoring all mailboxes.
 */
void 
Biff::stop_monitoring (void)
{
#ifdef DEBUG
	g_message ("Stop monitoring mailboxes");
#endif
	std::vector<class Mailbox *>::iterator mailbox;

	g_mutex_lock (mutex_);
	mailbox = mailbox_.begin ();
	while (mailbox != mailbox_.end ())
	  (*(mailbox++))->stop ();
	g_mutex_unlock (mutex_);
}

// ============================================================================
//  options
// ============================================================================

/**
 *  This function is called when an option is changed that has the
 *  OPTFLG_CHANGE flag set.
 *
 *  @param option Pointer to the option that is changed.
 */
void 
Biff::option_changed (Option *option)
{
	if (!option)
		return;

	// POPUP_FORMAT
	if (option->name() == "popup_format") {
		std::vector<guint> vec;
		((Option_String *)option)->get_vector (vec, ':');
		if (vec.size() < 3)
			return;
		value ("popup_size_sender", std::min<guint> (vec[0], 255), false);
		value ("popup_size_subject", std::min<guint> (vec[1], 255), false);
		value ("popup_size_date", std::min<guint> (vec[2], 255), false);
		return;
	}

	// POPUP_SIZE_SENDER, POPUP_SIZE_SUBJECT, POPUP_SIZE_DATE
	if ((option->name() == "popup_size_sender")
		|| (option->name() == "popup_size_subject")
		|| (option->name() == "popup_size_date")) {
		// Remark: Do not depend on these options, depend on "popup_format"
		// instead!
		std::stringstream ss;
		ss << value_uint ("popup_size_sender") << ":";
		ss << value_uint ("popup_size_subject") << ":";
		ss << value_uint ("popup_size_date");
		value ("popup_format", ss.str());
		return;
	}

	// UI_MODE
	if (option->name() == "ui_mode") {
		value ("gtk_mode", ((Option_UInt *)option)->value() == GTK_MODE);
		return;
	}
}

/**
 *  This function is called when an option is to be read that needs updating
 *  before. These options have to be marked by the OPTFLG_UPDATE flag.
 *
 *  @param option Pointer to the option that is to be updated.
 */
void 
Biff::option_update (Option *option)
{
}

/**
 *  The loaded config file belongs to an old version of gnubiff. All options
 *  with changed default values will be converted if possible. If the option
 *  still has the old default value this is no problem, otherwise we might
 *  give a warning message so the user can do this manually.
 *
 *  If the config file belongs to a newer version of gnubiff, no conversion
 *  will be done!
 */
void 
Biff::upgrade_options (void)
{
	// Get version of gnubiff binary
	guint gnubiff_version = Support::version_to_integer (PACKAGE_VERSION);

	// Get config file version and reset internal version
	std::string config_version = value_string ("version");
	guint version = 0;
	if (config_version == "0")
		config_version = "<=2.1.1";
	else
		version = Support::version_to_integer (config_version);
	reset ("version");

	// Check for newer version config file
	if (version > gnubiff_version) {
		g_warning (_("Loaded config file from newer gnubiff version \"%s\"."),
					 config_version.c_str ());
		return; 
	}
	if (version == gnubiff_version)
		return;

	// Config file belongs to an older version of gnubiff
	g_warning (_("Loaded config file from old gnubiff version \"%s\"."),
			   config_version.c_str ());
	g_message (_("Trying to convert all options."));

	// Store options that need manual conversion
	std::string options_bad;

	// Global options

	// Option: MIN_BODY_LINES
	if (version < 2001002) {
		if (value_uint ("min_body_lines") == 12)
			reset ("min_body_lines");
		else
			options_bad += "\"min_body_lines\", ";
	}

	// Mailbox options
	for (guint i = 0; i < get_number_of_mailboxes (); i++) {
		Mailbox *mb = mailbox(i);

		// Option: ADDRESS (for maildir protocol)
		if ((version < 2001003) && (mb->protocol() == PROTOCOL_MAILDIR)) {
			const gchar* address = mb->address().c_str();
			gchar *base = g_path_get_basename (address);
			if (base && (std::string(base) != "new")) {
				gchar *md_new = g_build_filename (address, "new", NULL);
				if (md_new)
					mb->address (md_new);
				g_free (md_new);
				g_free (base);
			}
		}
	}

	// End message
	if (options_bad.size() == 0)
		g_message (_("Successfully converted all options."));
	else {
		options_bad = options_bad.substr (0, options_bad.size()-2);
		g_warning (_("Successfully converted some options. The following "
					 "options must be updated manually: %s."),
				   options_bad.c_str());
	}
}

// ============================================================================
//  i/o
// ============================================================================

/**
 * Opens the new block {\em name} of options in the configuration file.
 *
 * @param  name  valid utf-8 character array for the name of the block
 */
void 
Biff::save_newblock (const gchar *name)
{
	save_blocks.push_back (name);
	const gchar *fmt = "%*s<%s>\n";
	gchar *esc = g_markup_printf_escaped(fmt,save_blocks.size()*2-2,"",name);
	save_file << esc;
	g_free (esc);
}
  	 
/**
 * Ends the last opened block of options in the configuration file.
 */
void 
Biff::save_endblock (void)
{
	const gchar *fmt = "%*s</%s>\n";
	gchar *esc = g_markup_printf_escaped(fmt, save_blocks.size()*2-2, "",
										 save_blocks[save_blocks.size()-1]);
	save_file << esc;
	g_free (esc);
	save_blocks.pop_back ();
}

/**
 *  Save all the given parameters into the configuration file.
 *
 *  @param map   Map of pairs (name, value) for the parameters to be saved.
 *  @param block Name of the XML tag that encloses the block of parameters in
 *               {\em map}. If it's the empty string (this is the default) no
 *               block is generated.
 */
void 
Biff::save_parameters (std::map<std::string,std::string> &map,
					   std::string block)
{
	const gchar *fmt = "%*s<parameter name=\"%s\"%*svalue=\"%s\"/>\n", *name;
	gchar *esc;
	if (block.size () > 0)
		save_newblock (block.c_str ());

	std::map<std::string,std::string>::iterator it = map.begin ();
	while (it != map.end ()) {
		name = it->first.c_str ();
		esc = g_markup_printf_escaped(fmt, save_blocks.size()*2, "", name,
									  28-strlen(name)-save_blocks.size()*2,
									  "", it->second.c_str ());
		save_file << esc;
		g_free(esc);
		it++;
	}

	if (block.size () > 0)
		save_endblock ();
}

/**
 *  Save all option and mailboxes to the config file. If no config file
 *  exists a new one is created that is readable only by the user.
 *
 *  @return       boolean indicating success
 */
gboolean 
Biff::save (void)
{
	// Note: "stringstream" and standard C file access functions are used
	// instead of "ofstream" because there seems to be no way to set file
	// permissions without the susceptibility to race conditions when using
	// "ofstream" (Does ofstream respect the umask function?).

	// XML header
	save_blocks.clear();
	save_file.str(std::string(""));
	save_file << "<?xml version=\"1.0\"?>" << std::endl;
	save_newblock("configuration-file");


	// Mailboxes
	std::map<std::string,std::string> name_value;
	g_mutex_lock (mutex_);
	for (unsigned int i=0; i< mailbox_.size(); i++) {
#ifdef USE_PASSWORD
		// Encrypt password
		mailbox_[i]->value ("password", Decoding::encrypt_password (mailbox_[i]->value_string ("password"), passtable_));
#endif
		// Save options
		mailbox_[i]->to_strings (OPTGRP_MAILBOX, name_value);
		save_parameters (name_value, "mailbox");
#ifdef USE_PASSWORD
		// Decrypt password
		mailbox_[i]->value ("password", Decoding::decrypt_password (mailbox_[i]->value_string ("password"), passtable_));
#endif
	}
	g_mutex_unlock (mutex_);

	// Save options common to all mailboxes (each group of options separate)
	std::map<guint, Option_Group *>::iterator it = groups()->begin();
	while (it != groups()->end()) {
		std::string name = it->second->name ();
		to_strings (it->first, name_value);
		it++;
		// Any options in this group to be saved?
		if (name_value.empty())
			continue;
		save_parameters (name_value, name);
	}

	// End Header
	save_endblock();

	// Write Configuration to file
	int fd = open (value_gchar ("config_file"), O_WRONLY | O_CREAT | O_TRUNC,
				   S_IRUSR | S_IWUSR);
	if (fd==-1)
	    return false;
	if (write(fd,save_file.str().c_str(),save_file.str().size())==-1)
	    return false;
	if (close(fd)==-1)
	    return false;

	return true;
}

gboolean 
Biff::load (void)
{
	// Reset version. This must be done to detect pre 2.1.2 config files
	value ("version", "0");

	mailbox_.clear();

	std::ifstream file;
	std::string line;
	file.open (value_gchar ("config_file"));
	if (!file.is_open()) {
		mailbox_.push_back (new Mailbox (this));
		g_warning (_("Cannot open your configuration file (%s)"),
				   value_gchar ("config_file"));
		return false;
	}

	// Instantiate a new xml parser
	GMarkupParser parser;
	parser.start_element = BIFF_xml_start_element;
	parser.end_element   = BIFF_xml_end_element;
	parser.text          = 0;
	parser.passthrough   = 0;
	parser.error         = BIFF_xml_error;
	GMarkupParseContext *context;
	context = g_markup_parse_context_new (&parser, GMarkupParseFlags (0),
										  this, 0);
	if (!context) {
		g_warning (_("Cannot create XML parser for config file"));
		return false;
	}

	// Parse the file
	gboolean status = TRUE;

	g_mutex_lock (mutex_);
	while ((!getline(file, line).eof()) && (status))
		status = g_markup_parse_context_parse (context, line.c_str(),
											   line.size(), 0);
	g_mutex_unlock (mutex_);

	g_markup_parse_context_free (context);

	// Check if we got at least one mailbox definition
	if (mailbox_.size() == 0) {
		g_warning (_("Found no mailbox definition in your configuration "
					 "file (%s)"), value_gchar ("config_file"));
		mailbox_.push_back (new Mailbox (this));
	}

	file.close ();

	// Do we have an config file from another version?
	if (value_string ("version") != PACKAGE_VERSION)
		upgrade_options ();

	return true;
}

/**
 *  Callback function when parsing the config file. This function is called
 *  when a new XML tag is parsed.
 *
 *  @param context          FIXME!
 *  @param element_name     Name of the XML tag
 *  @param attribute_name   array with the names of the attributes for the tag
 *  @param attribute_values array with the values of the attributes for the tag
 *  @param error            FIXME!
 */
void 
Biff::xml_start_element (GMarkupParseContext *context,
						 const gchar *element_name,
						 const gchar **attribute_names,
						 const gchar **attribute_values,
						 GError **error)
{
	// Test parameters
	if ((element_name == NULL) || (attribute_names == NULL)
		|| (attribute_values == NULL)) {
		unknown_internal_error ();
		return;
	}

	// All tags with the exception of the "parameter" tag start new 
	if (std::string (element_name) != "parameter")
		buffer_load_.clear ();
	else {
		std::map<std::string,std::string> temp;

		for (guint i = 0; attribute_names[i] != 0; i++)
			temp[attribute_names[i]] = attribute_values[i];
		if (temp["name"].empty ()) {
			g_warning (_("Illegal parameter format in config file"));
			return;
		}
		buffer_load_[temp["name"]] = temp["value"];
	}
}

void 
Biff::xml_end_element (GMarkupParseContext *context,
					   const gchar *element_name, GError **error)
{
	// Test parameters
	if (element_name == NULL) {
		unknown_internal_error ();
		return;
	}

	std::string element = element_name;

	// XML elements to be ignored
	if ((element == "parameter") || (element == "configuration-file"))
		return;

	// Mailbox
	if (element == "mailbox") {
		guint protocol = PROTOCOL_NONE, pos = mailbox_.size();

		// Need to get protocol first
		if (buffer_load_.find ("protocol") == buffer_load_.end())
			g_warning(_("No protocol specified for mailbox %d"), pos);
		else
			protocol = string_to_value ("protocol", buffer_load_["protocol"]);

		// Create mailbox
		switch (protocol) {
		case PROTOCOL_FILE:
			mailbox_.push_back (new File (this));
			break;
		case PROTOCOL_MH:
			mailbox_.push_back (new Mh (this));
			break;
		case PROTOCOL_MH_BASIC:
			mailbox_.push_back (new Mh_Basic (this));
			break;
		case PROTOCOL_MH_SYLPHEED:
			mailbox_.push_back (new Mh_Sylpheed (this));
			break;
		case PROTOCOL_MAILDIR:
			mailbox_.push_back (new Maildir (this));
			break;
		case PROTOCOL_IMAP4:
			mailbox_.push_back (new Imap4 (this));
			break;
		case PROTOCOL_POP3:
			mailbox_.push_back (new Pop3 (this));
			break;
		case PROTOCOL_APOP:
			mailbox_.push_back (new Apop (this));
			break;
		default:
			mailbox_.push_back (new Mailbox (this));
			break;
		}

		// Get options
		mailbox_[pos]->from_strings (OPTGRP_MAILBOX, buffer_load_);
#ifdef USE_PASSWORD
		// Decrypt password
		mailbox_[pos]->value ("password", Decoding::decrypt_password (mailbox_[pos]->value_string ("password"), passtable_));
#endif
	}
	// Options common to all mailboxes
	else
		from_strings (OPTGRP_ALL & (~OPTGRP_MAILBOX), buffer_load_);
}

void 
Biff::xml_error (GMarkupParseContext *context, GError *error)
{
	g_warning ("%s\n", error->message);
}
