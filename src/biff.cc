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

#include "support.h"

#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <map>
#include <unistd.h>

#ifdef USE_GNOME
#  include "ui-applet-gnome.h"
#endif

#include "biff.h"
#include "ui-authentication.h"
#include "ui-preferences.h"
#include "ui-properties.h"
#include "ui-applet-gtk.h"
#include "ui-popup.h"
#include "mailbox.h"
#include "file.h"
#include "maildir.h"
#include "mh.h"
#include "imap4.h"
#include "pop3.h"
#include "apop.h"


// ================================================================================
//  "C" binding
// ================================================================================
extern "C" {
	void BIFF_xml_start_element (GMarkupParseContext *context,
								 const gchar *element_name,
								 const gchar **attribute_names,
								 const gchar **attribute_values,
								 gpointer data,
								 GError **error)
	{
		BIFF(data)->xml_start_element (context, element_name, attribute_names, attribute_values, error);
	}
	void BIFF_xml_end_element (GMarkupParseContext *context,
							   const gchar *element_name,
							   gpointer data,
							   GError **error)
	{
		BIFF(data)->xml_end_element (context, element_name, error);
	}

	void BIFF_xml_error (GMarkupParseContext *context,
						 GError *error,
						 gpointer data)
	{
		BIFF(data)->xml_error (context, error);
	}
}

// ================================================================================
//  base
// ================================================================================
Biff::Biff (gint ui_mode,
			std::string filename)
{
	// Get password table from configure option
#ifdef USE_PASSWORD
	passtable_ = PASSWORD_STRING;
	// Add something (in case the provided string is not long enough)
	passtable_ += "FEDCBA9876543210";
	//  and then we remove duplicated characters
	std::string buffer;
	for (guint i=0; i<passtable_.size(); i++)
		if (buffer.find(passtable_[i]) == std::string::npos)
			buffer += passtable_[i];
	passtable_ = buffer;
#endif

	ui_mode_ 	= ui_mode;
	check_mode_	= AUTOMATIC_CHECK;

	// General
	use_max_mail_ = true;
	max_mail_ = 100;
	use_newmail_command_ = true;
	newmail_command_ = "play "GNUBIFF_DATADIR"/coin.wav";
	use_double_command_ = true;
	double_command_ = "xemacs";

	// Applet
	applet_use_geometry_	= true;
	applet_geometry_		= "+0+0";
	applet_use_decoration_	= false;
	applet_be_sticky_       = false;
	applet_keep_above_      = false;
	applet_pager_			= false;
	applet_font_			= "sans 10";
	use_newmail_text_		= true;
	newmail_text_			= "%d";
	use_newmail_image_		= true;
	newmail_image_			= GNUBIFF_DATADIR"/tux-awake.png";
	use_nomail_text_		= true;
	nomail_text_			= _("no mail");
	use_nomail_image_		= true;
	nomail_image_			= GNUBIFF_DATADIR"/tux-sleep.png";

	// Popup
	use_popup_				= true;	
	popup_delay_			= 5;
	popup_use_geometry_		= true;
	popup_be_sticky_        = false;
	popup_keep_above_       = false;
	popup_pager_			= false;
	popup_geometry_			= "-0+0";
	popup_font_				= "sans 10";
	popup_use_decoration_	= false;
	popup_use_size_			= true;
	popup_size_				= 40;
	popup_use_format_		= true;
	popup_format ("50:50:50");

	mutex_					= g_mutex_new ();

	// Do we have a valid configuration file?
	if (!filename.empty())
		filename_ = filename;
	else
	{
		gchar *filename=g_build_filename(g_get_home_dir (),".gnubiffrc", NULL);
		filename_ = std::string (filename);
		g_free(filename);
	}

	// Does this configuration file exist?
	std::ifstream file;
	file.open (filename_.c_str());
	if (file.is_open()) {
		file.close();
		load();
	}
	else {
		g_warning (_("Configuration file (%s) not found !"), filename_.c_str());
		mailbox_.push_back (new Mailbox (this));
	}

	// Applet
#ifdef USE_GNOME
	if (ui_mode_ == GNOME_MODE)
		applet_ = new AppletGnome (this);
	else
		applet_ = new AppletGtk (this);
#else
	applet_ = new AppletGtk (this);
#endif
	applet_->create();

	// Preferences
	preferences_ = new Preferences (this);
	preferences_->create ();

	// Popup
	popup_ = new Popup (this);
	popup_->create();

	// Authentication dialog
	ui_auth_mutex_=g_mutex_new ();
	ui_auth_ = new Authentication ();
}


Biff::~Biff (void)
{
}


// ================================================================================
//  access
// ================================================================================
void
Biff::popup_format (std::string format)
{
	std::string copy = format;

	// Remove any non numeric character
	for (guint i=0; i<copy.size(); i++) 
		if (!g_ascii_isdigit(copy[i]))
			copy[i] = ' ';

	std::istringstream strin(copy);
	strin >> sender_size_ >> subject_size_ >> date_size_;

	// Hard limit on field size
	if (sender_size_ > 255)
		sender_size_ = 255;
	if (subject_size_ > 255)
		subject_size_ = 255;
	if (date_size_ > 255)
		date_size_ = 255;

	popup_format_ = format;
}

guint
Biff::size (void)
{
	g_mutex_lock (mutex_);
	guint size = mailbox_.size();
	g_mutex_unlock (mutex_);
	return size;
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
	for (guint i=0; i<mailbox_.size(); i++)
		if (mailbox_[i]->uin() == uin) {
			find = mailbox_[i];
			break;
		}
	g_mutex_unlock (mutex_);
	return find;
}

void
Biff::add (Mailbox *mailbox)
{
	g_mutex_lock (mutex_);
	mailbox_.push_back (mailbox);
	g_mutex_unlock (mutex_);
}

Mailbox *
Biff::replace (Mailbox *from, Mailbox *to)
{
	Mailbox *inserted = 0;
	g_mutex_lock (mutex_);
	for(std::vector<Mailbox *>::iterator i = mailbox_.begin(); i != mailbox_.end(); i++)
		if ((*i) == from) {
			(*i) = to;

			if ((preferences_) && (preferences_->selected() == from))
				preferences_->selected (to);
			delete from;
			inserted = to;
			break;
		}
	g_mutex_unlock (mutex_);

	if ((inserted) && (!GTK_WIDGET_VISIBLE(preferences_->get())))
		inserted->threaded_start (3);	
	return inserted;
}

void
Biff::remove (Mailbox *mailbox)
{
	g_mutex_lock (mutex_);
	for(std::vector<Mailbox *>::iterator i = mailbox_.begin(); i != mailbox_.end(); i++)
		if ((*i) == mailbox) {
			mailbox_.erase(i);
			break;
		}
	g_mutex_unlock (mutex_);
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
Biff::password (Mailbox *m)
{
	// Do we know the password already?
	if (!m->password().empty())
		return true;

	// Remark: It's important to block thread before looking at other mailboxes
	//         since one is maybe asking (using gui) for this password.
	g_mutex_lock (ui_auth_mutex_);

	// Searching other mailboxes
#if DEBUG
	g_message ("[%d] Looking for password for %s@%s:%d", m->uin(),
			   m->username().c_str(), m->address().c_str(), m->port());
#endif

	for (guint i=0; i < size(); i++)
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
		ui_auth_->select (m);
		gdk_threads_leave ();
	}

	g_mutex_unlock (ui_auth_mutex_);
	return !m->password().empty();
}


// ================================================================================
//  i/o
// ================================================================================

/**
 * Opens the new block {\em name} of options in the configuration file.
 *
 * @param  name  valid utf-8 character array for the name of the block
 */
void 
Biff::save_newblock(const gchar *name)
{
	save_blocks.push_back(name);
	const gchar *fmt="%*s<%s>\n";
	gchar *esc=g_markup_printf_escaped(fmt,save_blocks.size()*2-2,"",name);
	save_file << esc;
	g_free(esc);
}
  	 
/**
 * Ends the last opened block of options in the configuration file.
 */
void 
Biff::save_endblock(void)
{
	const gchar *fmt="%*s</%s>\n";
	gchar *esc=g_markup_printf_escaped(fmt,save_blocks.size()*2-2,"",
									   save_blocks[save_blocks.size()-1]);
	save_file << esc;
	g_free(esc);
	save_blocks.pop_back();
}
  	 
/**
 * Saves the string {\em value} for the option {\em name} to the
 * configuration file.
 *
 * @param  name  name of the option
 * @param  value string to be saved
 */
void 
Biff::save_para (const gchar *name, const std::string value)
{
	const gchar *fmt="%*s<parameter name=\"%s\"%*svalue=\"%s\"/>\n";
	gchar *esc=g_markup_printf_escaped(fmt,save_blocks.size()*2,"",name,
									   28-strlen(name)-save_blocks.size()*2,
									   "",value.c_str());
	
	save_file << esc;
	g_free(esc);
}

/**
 * Saves the set {\em value} of strings for the option {\em name} to the
 * configuration file. The strings are concatenated and separated by spaces.
 *
 * @param  name  name of the option
 * @param  value Set of strings to be saved
 */
void 
Biff::save_para(const gchar *name, const std::set<std::string> &value)
{
	std::stringstream str;
	for (std::set<std::string>::iterator j=value.begin(); j!=value.end(); j++)
		str << *j << " ";
	save_para(name, str.str());
}

/**
 * Saves the boolean {\em value} for the option {\em name} to the
 * configuration file.
 *
 * @param  name  name of the option
 * @param  value boolean that will be saved
 */
void 
Biff::save_para (const gchar *name, gboolean value)
{
	value ? save_para (name, "true") : save_para (name, "false");
}
  	 
/**
 * Saves the unsigned integer {\em value} for the option {\em name} to the
 * configuration file.
 *
 * @param  name  name of the option
 * @param  value unsigned integer that will be saved
 */
void 
Biff::save_para (const gchar *name, guint value)
{
	std::stringstream value_str;
	value_str << value;
	save_para (name, value_str.str());
}

/**
 * Loads the option {\em name} and saves its value in the boolean variable
 * {\em var}.
 *
 * @param name  name of the option
 * @param var   reference to the variable that gets the value
 */
void 
Biff::load_para(const gchar *name, gboolean &var)
{
	if (buffer_load_.find (std::string(name)) == buffer_load_.end())
		g_warning(_("Parameter \"%s\" not present, using default value"),name);
	std::string value = buffer_load_[std::string(name)];

	if ((value == "1") || (value == "true"))
		var = true;
	else if ((value == "0") || (value == "false"))
		var = false;
	else
		g_warning (_("Illegal value \"%s\" for parameter \"%s\", using default value"),
				   buffer_load_[std::string(name)].c_str(), name);
}

/**
 * Loads the option {\em name} and saves its value in the unsigned integer
 * variable {\em var}.
 *
 * @param name  name of the option
 * @param var   reference to the variable that gets the value
 */
void 
Biff::load_para(const gchar *name, guint &var)
{
	guint temp;

	if (buffer_load_.find (std::string(name)) == buffer_load_.end())
		g_warning(_("Parameter \"%s\" not present, using default value"),name);
	std::istringstream strin(buffer_load_[std::string(name)]);
	if (strin >> temp)
		var = temp;
	else
		g_warning (_("Illegal value \"%s\" for parameter \"%s\", using default value"),
				   buffer_load_[std::string(name)].c_str(), name);
}

/**
 * Loads the option {\em name} and saves its value in the string variable
 * {\em var}.
 *
 * @param name  name of the option
 * @param var   reference to the variable that gets the value
 */
void 
Biff::load_para(const gchar *name, std::string &var)
{
	if (buffer_load_.find (std::string(name)) == buffer_load_.end())
		g_warning(_("Parameter \"%s\" not present, using default value"),name);
	var = buffer_load_[std::string(name)];
}

/**
 * Loads the option {\em name} and saves its value in the set of strings
 * variable {\em var}.
 *
 * @param name  name of the option
 * @param var   reference to the variable that gets the value
 */
void 
Biff::load_para(const gchar *name, std::set<std::string> &var)
{
	if (buffer_load_.find (std::string(name)) == buffer_load_.end())
		g_warning(_("Parameter \"%s\" not present, using default value"),name);
	std::istringstream strin (buffer_load_[std::string(name)]);
	std::string value;

	var.clear ();
	while (strin >> value)
		var.insert (value);
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

	g_mutex_lock (mutex_);
	// Mailboxes
	for (unsigned int i=0; i< mailbox_.size(); i++)
	{
		save_newblock ("mailbox");
		mailbox_[i]->save_data ();
		save_endblock();
	}
	g_mutex_unlock (mutex_);

	// General
	save_newblock("general");
	save_para("use_max_mail",use_max_mail_);
	save_para("max_mail",max_mail_);
	save_para("use_newmail_command",use_newmail_command_);
	save_para("newmail_command",newmail_command_);
	save_para("use_double_command",use_double_command_);
	save_para("double_command",double_command_);
	save_endblock();

	// Applet
	save_newblock("applet");
	save_para("applet_use_geometry",applet_use_geometry_);
	save_para("applet_geometry",applet_geometry_);
	save_para("applet_be_sticky",applet_be_sticky_);
	save_para("applet_keep_above",applet_keep_above_);
	save_para("applet_pager",applet_pager_);
	save_para("applet_use_decoration",applet_use_decoration_);
	save_para("applet_font",applet_font_);
	save_para("use_newmail_text",use_newmail_text_);
	save_para("newmail_text",newmail_text_);
	save_para("use_newmail_image",use_newmail_image_);
	save_para("newmail_image",newmail_image_);
	save_para("use_nomail_text",use_nomail_text_);
	save_para("nomail_text",nomail_text_);
	save_para("use_nomail_image",use_nomail_image_);
	save_para("nomail_image",nomail_image_);
	save_endblock();

	// Popup
	save_newblock("popup");
	save_para("use_popup",use_popup_);
	save_para("popup_delay",popup_delay_);
	save_para("popup_use_geometry",popup_use_geometry_);
	save_para("popup_geometry",popup_geometry_);
	save_para("popup_use_decoration",popup_use_decoration_);
	save_para("popup_be_sticky",popup_be_sticky_);
	save_para("popup_keep_above",popup_keep_above_);
	save_para("popup_pager",popup_pager_);
	save_para("popup_font",popup_font_);
	save_para("popup_use_size",popup_use_size_);
	save_para("popup_size",popup_size_);
	save_para("popup_use_format",popup_use_format_);
	save_para("popup_format",popup_format_);
	save_endblock();

	// End Header
	save_endblock();

	// Write Configuration to file
	int fd=open(filename_.c_str(),O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR);
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
	mailbox_.clear();

	std::ifstream file;
	std::string line;
	file.open (filename_.c_str());
	if (!file.is_open()) {
		mailbox_.push_back (new Mailbox (this));
		g_warning (_("Cannot open your configuration file (%s)"), filename_.c_str());
		return false;
	}

	// Instantiate a new xml parser
	GMarkupParser parser;
	parser.start_element = BIFF_xml_start_element;
	parser.end_element   = BIFF_xml_end_element;
	parser.text          = 0;
	parser.passthrough   = 0;
	parser.error         = BIFF_xml_error;
	GMarkupParseContext *context = g_markup_parse_context_new (&parser, GMarkupParseFlags (0), this, 0);

	// Parse the file
	gboolean status = TRUE;

	g_mutex_lock (mutex_);
	while ((!getline(file, line).eof()) && (status))
		status = g_markup_parse_context_parse (context, line.c_str(), line.size(), 0);
	g_mutex_unlock (mutex_);

	g_markup_parse_context_free (context);

	// Check if we got at least one mailbox definition
	if (mailbox_.size() == 0) {
		g_warning (_("Found no mailbox definition in your configuration file (%s)"), filename_.c_str());
		mailbox_.push_back (new Mailbox (this));
	}

	file.close ();

	return true;
}

void
Biff::xml_start_element (GMarkupParseContext *context,
						 const gchar *element_name,
						 const gchar **attribute_names,
						 const gchar **attribute_values,
						 GError **error)
{
	if (std::string (element_name) != "parameter")
		buffer_load_.clear ();
	else {
		std::map<std::string,std::string> temp;

		for (guint i = 0; attribute_names[i]; i++)
			temp[attribute_names[i]] = attribute_values[i];
		if (temp["name"].empty ()) {
			g_warning(_("Illegal parameter format in config file"));
			return;
		}
		buffer_load_[temp["name"]] = temp["value"];
	}
}

void 
Biff::xml_end_element (GMarkupParseContext *context,
					   const gchar *element_name, GError **error)
{
	std::string element = element_name;

	// Mailbox
	if (element == "mailbox") {
		guint protocol;

		load_para ("protocol", protocol);
		switch (protocol) {
		case PROTOCOL_FILE:
			mailbox_.push_back (new File (this));
			break;
		case PROTOCOL_MH:
			mailbox_.push_back (new Mh (this));
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
		mailbox_[mailbox_.size()-1]->load_data ();
	}

	// General
	else if (element == "general") {
		load_para ("use_max_mail", use_max_mail_);
		load_para ("max_mail", max_mail_);
		load_para ("use_newmail_command", use_newmail_command_);
		load_para ("newmail_command", newmail_command_);
		load_para ("use_double_command", use_double_command_);
		load_para ("double_command", double_command_);
	}

	// Applet
	else if (element == "applet") {
		load_para ("applet_use_geometry", applet_use_geometry_);
		load_para ("applet_geometry", applet_geometry_);
		load_para ("applet_be_sticky", applet_be_sticky_);
		load_para ("applet_keep_above", applet_keep_above_);
		load_para ("applet_pager", applet_pager_);
		load_para ("applet_use_decoration", applet_use_decoration_);
		load_para ("applet_font", applet_font_);
		load_para ("use_newmail_text", use_newmail_text_);
		load_para ("newmail_text", newmail_text_);
		load_para ("use_newmail_image", use_newmail_image_);
		load_para ("newmail_image", newmail_image_);
		load_para ("use_nomail_text", use_nomail_text_);
		load_para ("nomail_text", nomail_text_);
		load_para ("use_nomail_image", use_nomail_image_);
		load_para ("nomail_image", nomail_image_);

		if (!g_file_test (newmail_image_.c_str(), G_FILE_TEST_EXISTS))
			newmail_image_ = GNUBIFF_DATADIR"/tux-awake.png";
		if (!g_file_test (nomail_image_.c_str(), G_FILE_TEST_EXISTS))
			nomail_image_ = GNUBIFF_DATADIR"/tux-sleep.png";
	}

	// Popup
	else if (element == "popup") {
		load_para ("use_popup", use_popup_);
		load_para ("popup_delay", popup_delay_);
		load_para ("popup_use_geometry", popup_use_geometry_);
		load_para ("popup_geometry", popup_geometry_);
		load_para ("popup_use_decoration", popup_use_decoration_);
		load_para ("popup_be_sticky", popup_be_sticky_);
		load_para ("popup_keep_above", popup_keep_above_);
		load_para ("popup_pager", popup_pager_);
		load_para ("popup_font", popup_font_);
		load_para ("popup_use_size", popup_use_size_);
		load_para ("popup_size", popup_size_);
		load_para ("popup_use_format", popup_use_format_);
		load_para ("popup_format", popup_format_);
	}
}

void 
Biff::xml_error (GMarkupParseContext *context, GError *error)
{
	g_warning ("%s\n", error->message);
}
