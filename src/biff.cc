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

#include <fstream>
#include <sstream>
#include <map>

#ifdef USE_GNOME
#  include "ui-applet-gnome.h"
#endif

#include "biff.h"
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
	popup_geometry_			= "-0+0";
	popup_font_				= "sans 10";
	popup_use_decoration_	= false;
	popup_use_size_			= true;
	popup_size_				= 40;
	popup_use_format_		= true;
	popup_format ("50:50:50");

	mutex_					= g_mutex_new ();

	// Do we have a valid configuration file ?
	if (!filename.empty())
		filename_ = filename;
	else 
		filename_ = std::string (g_get_home_dir ()) + std::string ("/.gnubiffrc");

	// Does this configuration file exist ?
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

// ================================================================================
//  i/o
// ================================================================================
gboolean
Biff::save (void)
{
	std::ofstream file;

	file.open (filename_.c_str());
	if (!file.is_open())
		return false;
  
	file << "<?xml version=\"1.0\"?>" << std::endl;
	file << "<configuration-file>"<< std::endl;

	g_mutex_lock (mutex_);
	// Mailboxes
	for (unsigned int i=0; i< mailbox_.size(); i++) {
		file << "  <mailbox>" << std::endl;
		file << "    <parameter name=\"protocol\"           value=\"" << mailbox_[i]->protocol() << "\"/>" << std::endl;
		file << "    <parameter name=\"authentication\"     value=\"" << mailbox_[i]->authentication() << "\"/>" << std::endl;
		file << "    <parameter name=\"name\"               value=\"" << mailbox_[i]->name() << "\"/>" << std::endl;
		file << "    <parameter name=\"address\"            value=\"" << mailbox_[i]->address() << "\"/>" << std::endl;
		file << "    <parameter name=\"username\"           value=\"" << mailbox_[i]->username() << "\"/>" << std::endl;
		// pop3 and imap4 protocols requires password in clear so we have to save password
		// in clear within configuration file. No need to say this is higly unsecure if
		// somebody looks at the file. So we try to take some measures:
		//   1. The file is made readable by owner only.
		//   2. password is "crypted" so it's not directly human readable.
		// Of course, these measures won't prevent a determined person to break in but will
		// at least prevent "ordinary" people to be able to steal password easily.
#ifdef USE_PASSWORD
		file << "    <parameter name=\"password\"           value=\"";
		for (guint j=0; j<mailbox_[i]->password().size(); j++)
			file << passtable_[mailbox_[i]->password()[j]/16] << passtable_[mailbox_[i]->password()[j]%16];
		file << "\"/>" << std::endl;
#else
		file << "    <parameter name=\"password\"           value=\"" << "" << "\"/>" << std::endl;
#endif
		file << "    <parameter name=\"port\"               value=\"" << mailbox_[i]->port() << "\"/>" << std::endl;
		file << "    <parameter name=\"folder\"             value=\"" << mailbox_[i]->folder() << "\"/>" << std::endl;
		file << "    <parameter name=\"certificate\"        value=\"" << mailbox_[i]->certificate() << "\"/>" << std::endl;
		file << "    <parameter name=\"delay\"              value=\"" << mailbox_[i]->delay() << "\"/>" << std::endl;
		file << "    <parameter name=\"use_other_folder\"   value=\"" << mailbox_[i]->use_other_folder() << "\"/>" << std::endl;
		file << "    <parameter name=\"other_folder\"       value=\"" << mailbox_[i]->other_folder() << "\"/>" << std::endl;
		file << "    <parameter name=\"use_other_port\"     value=\"" << mailbox_[i]->use_other_port() << "\"/>" << std::endl;
		file << "    <parameter name=\"other_port\"         value=\"" << mailbox_[i]->other_port() << "\"/>" << std::endl;
		file << "    <parameter name=\"seen\"               value=\"";
		for (guint j=0; j<mailbox_[i]->hiddens(); j++)
			file << mailbox_[i]->hidden(j) << " ";
		file << "\"/>" << std::endl;
		file << "  </mailbox>" << std::endl;
	}
	g_mutex_unlock (mutex_);

	// General
	file << "  <general>" << std::endl;
	file << "    <parameter name=\"use_max_mail\"          value=\"" << use_max_mail_ << "\"/>" << std::endl;
	file << "    <parameter name=\"max_mail\"              value=\"" << max_mail_ << "\"/>" << std::endl;
	file << "    <parameter name=\"use_newmail_command\"   value=\"" << use_newmail_command_ << "\"/>" << std::endl;
	file << "    <parameter name=\"newmail_command\"       value=\"" << newmail_command_ << "\"/>" << std::endl;
	file << "    <parameter name=\"use_double_command\"    value=\"" << use_double_command_ << "\"/>" << std::endl;
	file << "    <parameter name=\"double_command\"        value=\"" << double_command_ << "\"/>" << std::endl;
	file << "  </general>" << std::endl;

	// Applet
	file << "  <applet>" << std::endl;
	file << "    <parameter name=\"applet_use_geometry\"   value=\"" << applet_use_geometry_ << "\"/>" << std::endl;
	file << "    <parameter name=\"applet_geometry\"       value=\"" << applet_geometry_ << "\"/>" << std::endl;
	file << "    <parameter name=\"applet_use_decoration\" value=\"" << applet_use_decoration_ << "\"/>" << std::endl;
	file << "    <parameter name=\"applet_font\"           value=\"" << applet_font_ << "\"/>" << std::endl;
	file << "    <parameter name=\"use_newmail_text\"      value=\"" << use_newmail_text_ << "\"/>" << std::endl;
	file << "    <parameter name=\"newmail_text\"          value=\"" << newmail_text_ << "\"/>" << std::endl;
	file << "    <parameter name=\"use_newmail_image\"     value=\"" << use_newmail_image_ << "\"/>" << std::endl;
	file << "    <parameter name=\"newmail_image\"         value=\"" << newmail_image_ << "\"/>" << std::endl;
	file << "    <parameter name=\"use_nomail_text\"       value=\"" << use_nomail_text_ << "\"/>" << std::endl;
	file << "    <parameter name=\"nomail_text\"           value=\"" << nomail_text_ << "\"/>" << std::endl;
	file << "    <parameter name=\"use_nomail_image\"      value=\"" << use_nomail_image_ << "\"/>" << std::endl;
	file << "    <parameter name=\"nomail_image\"          value=\"" << nomail_image_ << "\"/>" << std::endl;
	file << "  </applet>" << std::endl;

	// Popup
	file << "  <popup>" << std::endl;
	file << "    <parameter name=\"use_popup\"             value=\"" << use_popup_  << "\"/>" << std::endl;
	file << "    <parameter name=\"popup_delay\"           value=\"" << popup_delay_ << "\"/>" << std::endl;
	file << "    <parameter name=\"popup_use_geometry\"    value=\"" << popup_use_geometry_ << "\"/>" << std::endl;
	file << "    <parameter name=\"popup_geometry\"        value=\"" << popup_geometry_ << "\"/>" << std::endl;
	file << "    <parameter name=\"popup_use_decoration\"  value=\"" << popup_use_decoration_  << "\"/>" << std::endl;
	file << "    <parameter name=\"popup_font\"            value=\"" << popup_font_ << "\"/>" << std::endl;
	file << "    <parameter name=\"popup_use_size\"        value=\"" << popup_use_size_  << "\"/>" << std::endl;
	file << "    <parameter name=\"popup_size\"            value=\"" << popup_size_  << "\"/>" << std::endl;
	file << "    <parameter name=\"popup_use_format\"      value=\"" << popup_use_format_  << "\"/>" << std::endl;
	file << "    <parameter name=\"popup_format\"          value=\"" << popup_format_  << "\"/>" << std::endl;
	file << "  </popup>" << std::endl;


	// End Header
	file << "</configuration-file>"<< std::endl;

	// Close the file
	file.close ();

	// Restrict file permission
	std::string command = std::string("chmod 600 ") + filename_;
	system (command.c_str());
  
	return true;
}


gboolean
Biff::load (void)
{
	count_ = -1;
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
	parser.end_element   = 0;
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
	if (std::string (element_name) == "mailbox")
		count_++;

	if (std::string (element_name) == "parameter") {
		// Store attributes (name & value) in a map
	    std::map<std::string,std::string> fmap;
		guint i = 0;
		while (attribute_names[i]) {
			fmap[attribute_names[i]] = attribute_values[i];
			i++;
		}

		// A new mailbox can be created once we know the protocol
		//  so this is the first information to be saved and loaded.
		if (fmap["name"] == "protocol") {
			int protocol;
			std::istringstream strin(fmap["value"]);
			strin >> protocol;
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
		}

		//
		// Mailbox
		//
		if (fmap["name"] == "name") {
			mailbox_[count_]->name (fmap["value"]);
		}
		
		else if (fmap["name"] == "authentication") {
			std::istringstream strin(fmap["value"]);
			guint value; strin >> value;
			mailbox_[count_]->authentication (value);
		}

		else if (fmap["name"] == "address") {
			mailbox_[count_]->address (fmap["value"]);
		}

		else if (fmap["name"] == "username") {
			mailbox_[count_]->username (fmap["value"]);
		}

		else if (fmap["name"] == "password") {
			std::string password;
			std::string tmp = fmap["value"];
			for (gint i=0; i<gint(tmp.size())-1; i+=2) {
				char c = 0;
				guint j;
				for (j=0; j<16; j++) {
					if (passtable_ [j] == tmp[i])
						c += j*16;
					if (passtable_ [j] == tmp[i+1])
						c += j;
				}
				password += c;
			}
			mailbox_[count_]->password(password);
		}

		else if (fmap["name"] == "port") {
			std::istringstream strin(fmap["value"]);
			guint value; strin >> value;
			mailbox_[count_]->port(value);
		}


		else if (fmap["name"] == "folder") {
			mailbox_[count_]->folder (fmap["value"]);
		}

		else if (fmap["name"] == "certificate") {
			mailbox_[count_]->certificate (fmap["value"]);
		}

		else if (fmap["name"] == "delay") {
			std::istringstream strin(fmap["value"]);
			guint value; strin >> value;
			mailbox_[count_]->delay (value);
		}

		else if (fmap["name"] == "use_other_folder") {
			std::istringstream strin(fmap["value"]);
			guint value; strin >> value;
			mailbox_[count_]->use_other_folder(value);
		}

		else if (fmap["name"] == "other_folder") {
			mailbox_[count_]->other_folder (fmap["value"]);
		}

		else if (fmap["name"] == "use_other_port") {
			std::istringstream strin(fmap["value"]);
			guint value; strin >> value;
			mailbox_[count_]->use_other_port(value);
		}

		else if (fmap["name"] == "other_port") {
			std::istringstream strin(fmap["value"]);
			guint value; strin >> value;
			mailbox_[count_]->other_port(value);
		}


		else if (fmap["name"] == "seen") {
			std::istringstream strin (fmap["value"]);
			mailbox_[count_]->hidden().clear();
			guint mailid;
			while (strin >> mailid)
				mailbox_[count_]->hidden().push_back (mailid);
		}

		//
		// General
		//
		else if (fmap["name"] == "use_max_mail") {
			std::istringstream strin(fmap["value"]);
			strin >> use_max_mail_;
		}

		else if (fmap["name"] == "max_mail") {
			std::istringstream strin(fmap["value"]);
			strin >> max_mail_;
		}

		else if (fmap["name"] == "use_newmail_command") {
			std::istringstream strin(fmap["value"]);
			strin >> use_newmail_command_;
		}

		else if (fmap["name"] == "newmail_command") {
			newmail_command_ = fmap["value"];
		}

		else if (fmap["name"] == "use_double_command") {
			std::istringstream strin(fmap["value"]);
			strin >> use_double_command_;
		}

		else if (fmap["name"] == "double_command") {
			double_command_ = fmap["value"];
		}

		else if (fmap["name"] == "check_mode") {
			std::istringstream strin(fmap["value"]);
			strin >> check_mode_;
		}

		//
		// Applet
		//
		else if (fmap["name"] == "applet_use_geometry") {
			std::istringstream strin(fmap["value"]);
			strin >> applet_use_geometry_;
		}

		else if (fmap["name"] == "applet_geometry") {
			applet_geometry_ = fmap["value"];
		}

		else if (fmap["name"] == "applet_use_decoration") {
			std::istringstream strin(fmap["value"]);
			strin >> applet_use_decoration_;
		}

		else if (fmap["name"] == "applet_font")
			applet_font_ = fmap["value"];


		else if (fmap["name"] == "use_newmail_text") {
			std::istringstream strin(fmap["value"]);
			strin >> use_newmail_text_;
		}

		else if (fmap["name"] == "newmail_text")
			newmail_text_ = fmap["value"];

		else if (fmap["name"] == "use_newmail_image") {
			std::istringstream strin(fmap["value"]);
			strin >> use_newmail_image_;
		}

		else if (fmap["name"] == "newmail_image") {
			newmail_image_ = fmap["value"];
			if (!g_file_test (newmail_image_.c_str(), G_FILE_TEST_EXISTS))
				newmail_image_ = GNUBIFF_DATADIR"/tux-awake.png";
		}

		else if (fmap["name"] == "use_nomail_text") {
			std::istringstream strin(fmap["value"]);
			strin >> use_nomail_text_;
		}

		else if (fmap["name"] == "nomail_text")
			 nomail_text_ = fmap["value"];

		else if (fmap["name"] == "use_nomail_image") {
			std::istringstream strin(fmap["value"]);
			strin >> use_nomail_image_;
		}

		else if (fmap["name"] == "nomail_image") {
			nomail_image_ = fmap["value"];
			if (!g_file_test (nomail_image_.c_str(), G_FILE_TEST_EXISTS))
				nomail_image_ = GNUBIFF_DATADIR"/tux-sleep.png";
		}

		//
		// Popup
		//
		else if (fmap["name"] == "use_popup") {
			std::istringstream strin(fmap["value"]);
			strin >> use_popup_;
		}

		else if (fmap["name"] == "popup_delay") {
			std::istringstream strin(fmap["value"]);
			strin >> popup_delay_;
		}

		else if (fmap["name"] == "popup_use_geometry") {
			std::istringstream strin(fmap["value"]);
			strin >> popup_use_geometry_;
		}

		else if (fmap["name"] == "popup_geometry") {
			popup_geometry_ = fmap["value"];
		}

		else if (fmap["name"] == "popup_use_decoration") {
			std::istringstream strin(fmap["value"]);
			strin >> popup_use_decoration_;
		}

		else if (fmap["name"] == "popup_font") {
			popup_font_ = fmap["value"];
		}

		else if (fmap["name"] == "popup_use_size") {
			std::istringstream strin(fmap["value"]);
			strin >> popup_use_size_;
		}

		else if (fmap["name"] == "popup_size") {
			std::istringstream strin(fmap["value"]);
			strin >> popup_size_;
		}

		else if (fmap["name"] == "popup_use_format") {
			std::istringstream strin(fmap["value"]);
			strin >> popup_use_format_;
		}

		else if (fmap["name"] == "popup_format") {
			popup_format (fmap["value"]);
		}
	}
}

void
Biff::xml_error (GMarkupParseContext *context,
				 GError *error)
{
	g_warning ("%s\n", error->message);
}
