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

#include <fstream>
#include <sstream>
#include <map>
#include <fcntl.h>
#include "biff.h"
#include "ui-preferences.h"
#include "ui-properties.h"
#ifdef USE_GNOME
#  include "ui-applet-gnome.h"
#endif
#include "ui-applet-gtk.h"
#include "ui-popup.h"
#include "mailbox.h"
#include "file.h"
#include "maildir.h"
#include "mh.h"
#include "imap4.h"
#include "pop3.h"
#include "apop.h"
#include "nls.h"


/**
 * "C" binding
**/
extern "C" {
	gpointer BIFF_lookup (gpointer data)
	{
		BIFF(data)->lookup_thread ();
		return 0;
	}

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


Biff::Biff (gint ui_mode,
			std::string filename)
{
	lookup_mutex_ = g_mutex_new ();

#ifdef USE_PASSWORD
	passtable_ = PASSWORD_STRING;
	// Add something in case provided string is not long enough
	passtable_ += "FEDCBA9876543210";
	// Now, we remove duplicated characters
	std::string buffer;
	for (guint i=0; i<passtable_.size(); i++)
		if (buffer.find(passtable_[i]) == std::string::npos)
			buffer += passtable_[i];
	passtable_ = buffer;
#endif

	ui_mode_ = ui_mode;

	// Defaults
	no_clear_password_		= true;
	sound_type_				= SOUND_FILE;
	sound_command_ 			= "play %s -v %v";
	sound_file_				= GNUBIFF_DATADIR"/coin.wav";
	sound_volume_			= 50;
	check_mode_				= AUTOMATIC_CHECK;
	max_mail_				= 100;
	mail_app_				= "xemacs";
	
	popup_display_			= true;
	popup_time_				= 5;
	popup_use_geometry_		= true;
	popup_geometry_			= "-0+0";
	popup_is_decorated_		= false;
	popup_max_line_			= 40;
	popup_max_sender_size_	= 100;
	popup_max_subject_size_	= 100;
	popup_display_date_		= false;
	popup_font_				= "sans 8";
	popup_font_color_		= "black";
	popup_back_color_		= "white";

	biff_newmail_image_		= GNUBIFF_DATADIR"/tux-awake.png";
	biff_nomail_image_		= GNUBIFF_DATADIR"/tux-sleep.png";
	biff_use_newmail_image_ = true;
	biff_use_nomail_image_	= true;
	biff_use_newmail_text_	= true;
	biff_use_nomail_text_	= true;
	biff_newmail_text_		= "%d";
	biff_nomail_text_		= "%d";
	biff_use_geometry_		= true;
	biff_geometry_			= "+0+0";
	biff_is_decorated_		= false;
	biff_font_				= "sans 16";
	biff_font_color_		= "black";


	if (!filename.empty())
		filename_ = filename;
	else 
		filename_ = std::string (g_get_home_dir ()) + std::string ("/.gnubiffrc");

	// Does configuration file exist ?
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

Mailbox *
Biff::mailbox (guint index)
{
	if (index < mailbox_.size())
		return mailbox_[index];
	return 0;
}

Mailbox *
Biff::find (guint uin)
{
	for (guint i=0; i<mailbox_.size(); i++)
		if (mailbox_[i]->uin() == uin)
			return mailbox_[i];
	return 0;
}

void
Biff::remove (guint uin)
{
	for(std::vector<Mailbox *>::iterator i = mailbox_.begin(); i != mailbox_.end(); i++)
		if ((*i)->uin() == uin) {
			mailbox_.erase(i);
			return;
		}
}

void
Biff::add (Mailbox *mailbox)
{
	mailbox_.push_back (mailbox);
}

void
Biff::lookup (void)
{
	GError *err = NULL;
	g_thread_create (BIFF_lookup, this, FALSE, &err);
	if (err != NULL)  {
		g_warning (_("Unable to create lookup thread: %s\n"), err->message);
		g_error_free (err);
	} 
}


void
Biff::lookup (Mailbox *mailbox)
{
	g_mutex_lock (lookup_mutex_);
	// Look for mailbox
	for (guint i=0; i < mailbox_.size(); i++)
		if (mailbox_[i] == mailbox) {
			lookup (i);
			g_mutex_unlock (lookup_mutex_);
			return;
		}
	g_mutex_unlock (lookup_mutex_);
}

void
Biff::lookup (guint index)
{
#ifdef DEBUG
	g_message ("[%d] Lookup mailbox format\n", mailbox_[index]->uin());
#endif

	guint protocol = mailbox_[index]->protocol ();
	mailbox_[index]->lookup_thread ();
	Mailbox *mailbox = 0;
	if (protocol != mailbox_[index]->protocol ()) {
		switch (mailbox_[index]->protocol ()) {
		case PROTOCOL_FILE:
			mailbox = new File (*mailbox_[index]);
			break;
		case PROTOCOL_MH:
			mailbox = new Mh (*mailbox_[index]);
			break;
		case PROTOCOL_MAILDIR:
			mailbox = new Maildir (*mailbox_[index]);
			break;
		case PROTOCOL_IMAP4:
			mailbox = new Imap4 (*mailbox_[index]);
			break;
		case PROTOCOL_POP3:
			mailbox = new Pop3 (*mailbox_[index]);
			break;
		case PROTOCOL_APOP:
#         ifdef HAVE_CRYPTO
			mailbox = new Apop (*mailbox_[index]);
#         else
			mailbox = new Pop3 (*mailbox_[index]);
#         endif
			break;
		case PROTOCOL_NONE:
			mailbox = new Mailbox (*mailbox_[index]);
			mailbox->status (MAILBOX_UNKNOWN);
			break;
		}
		// If "properties" currently displays mailbox_[index]
		//  we have to make it display the new mailbox
		if (preferences_->properties()->mailbox() == mailbox_[index])
			{
				gdk_threads_enter();
				preferences_->properties()->select (mailbox);
				gdk_threads_leave();
			}
		
		// Delete old mailbox and replace it with the new one
		Mailbox *old_mailbox = mailbox_[index];
		mailbox_[index] = mailbox;
		delete old_mailbox;
	}
	gdk_threads_enter();
	preferences_->synchronize();
	gdk_threads_leave();

	if ((mailbox) && (mailbox->protocol() != PROTOCOL_NONE) && (check_mode_ == AUTOMATIC_CHECK))
		mailbox->watch_on (mailbox->polltime());
}

void
Biff::lookup_thread (void)
{
	if (!g_mutex_trylock (lookup_mutex_))
		return;
	for (guint i=0; i < mailbox_.size(); i++)
		lookup (i);
	g_mutex_unlock (lookup_mutex_);
}

std::string
Biff::save_para(const gchar *name,std::string value,guint indent)
{
	const gchar *fmt="%*s<parameter name=\"%s\"%*svalue=\"%s\"/>\n";
	gchar *esc=g_markup_printf_escaped(fmt,indent,"",name,
									   28-strlen(name)-indent,"",
									   value.c_str());

	std::string result(esc);
	g_free(esc);
	return result;
}

std::string
Biff::save_para(const gchar *name,gint value,guint indent)
{
	const gchar *fmt="%*s<parameter name=\"%s\"%*svalue=\"%d\"/>\n";
	gchar *esc=g_markup_printf_escaped(fmt,indent,"",name,
									   28-strlen(name)-indent,"",value);

	std::string result(esc);
	g_free(esc);
	return result;
}

gboolean
Biff::save (void)
{
	// Note: "stringstream" and standard C file access functions are used
	// instead of "ofstream" because there seems to be no way to set file
	// permissions without the susceptibility to race conditions when using
	// "ofstream". Does ofstream respect the umask command? RSo

	std::stringstream file;

	file << "<?xml version=\"1.0\"?>" << std::endl;
	file << "<configuration-file>"<< std::endl;

	// Mailboxes
	for (unsigned int i=0; i< mailbox_.size(); i++)
	{
		file << "  <mailbox>" << std::endl;
		file << save_para("protocol",mailbox_[i]->protocol());
		file << save_para("name",mailbox_[i]->name());
		file << save_para("is_local",mailbox_[i]->is_local());
		file << save_para("location",mailbox_[i]->location());
		file << save_para("hostname",mailbox_[i]->hostname());
		file << save_para("port",mailbox_[i]->port());
		file << save_para("folder",mailbox_[i]->folder());
		file << save_para("username",mailbox_[i]->username());
		// pop3 and imap4 protocols requires password in clear so we have to
		// save password in clear within configuration file. No need to say
		// this is higly unsecure if somebody looks at the file. So we try to
		// take some measures:
		//   1. The file is made readable by owner only.
		//   2. password is "crypted" so it's not directly human readable.
		// Of course, these measures won't prevent a determined person to break
		// in but will at least prevent "ordinary" people to be able to steal
		// password easily.
#ifdef USE_PASSWORD
		std::stringstream password;
		for (guint j=0; j<mailbox_[i]->password().size(); j++)
		    password << passtable_[mailbox_[i]->password()[j]/16]
			         << passtable_[mailbox_[i]->password()[j]%16];
		file << save_para("password",password.str());
#else
		file << save_para("password",std::string(""));
#endif
		file << save_para("use_ssl",mailbox_[i]->use_ssl());
		file << save_para("certificate",mailbox_[i]->certificate());
		file << save_para("polltime",mailbox_[i]->polltime());
		std::stringstream seen;
		for (guint j=0; j<mailbox_[i]->hiddens(); j++)
		    seen << mailbox_[i]->hidden(j) << " ";
		file << save_para("seen",seen.str());
		file << "  </mailbox>" << std::endl;
	}

	// General
	file << "  <general>" << std::endl;
	file << save_para("no_clear_password",no_clear_password_);
	file << save_para("sound_type",sound_type_);
	file << save_para("sound_command",sound_command_);
	file << save_para("sound_file",sound_file_);
	file << save_para("sound_volume",sound_volume_);
	file << save_para("check_mode",check_mode_);
	file << save_para("max_mail",max_mail_);
	file << save_para("mail_app",mail_app_);
	file << "  </general>" << std::endl;

	// Popup
	file << "  <popup>" << std::endl;
	file << save_para("popup_display",popup_display_);
	file << save_para("popup_time",popup_time_);
	file << save_para("popup_use_geometry",popup_use_geometry_);
	file << save_para("popup_geometry",popup_geometry_);
	file << save_para("popup_is_decorated",popup_is_decorated_);
	file << save_para("popup_max_line",popup_max_line_);
	file << save_para("popup_max_sender_size",popup_max_sender_size_);
	file << save_para("popup_max_subject_size",popup_max_subject_size_);
	file << save_para("popup_display_date",popup_display_date_);
	file << save_para("popup_font",popup_font_);
	file << save_para("popup_font_color",popup_font_color_);
	file << "  </popup>" << std::endl;

	// Biff
	file << "  <biff>" << std::endl;
	file << save_para("biff_newmail_image",biff_newmail_image_);
	file << save_para("biff_nomail_image",biff_nomail_image_);
	file << save_para("biff_use_newmail_image",biff_use_newmail_image_);
	file << save_para("biff_use_nomail_image",biff_use_nomail_image_);
	file << save_para("biff_use_newmail_text",biff_use_newmail_text_);
	file << save_para("biff_use_nomail_text",biff_use_nomail_text_);
	file << save_para("biff_newmail_text",biff_newmail_text_);
	file << save_para("biff_nomail_text",biff_nomail_text_);
	file << save_para("biff_use_geometry",biff_use_geometry_);
	file << save_para("biff_geometry",biff_geometry_);
	file << save_para("biff_is_decorated",biff_is_decorated_);
	file << save_para("biff_font",biff_font_);
	file << save_para("biff_font_color",biff_font_color_);
	file << "  </biff>" << std::endl;

	// End Header
	file << "</configuration-file>" << std::endl;

	// Write Configuration to file
	int fd=open(filename_.c_str(),O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR);
	if (fd==-1)
	    return false;
	if (write(fd,file.str().c_str(),file.str().size())==-1)
	    return false;
	if (close(fd)==-1)
	    return false;

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
	while ((!getline(file, line).eof()) && (status))
		status = g_markup_parse_context_parse (context, line.c_str(), line.size(), 0);

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

		else if (fmap["name"] == "is_local") {
			std::istringstream strin(fmap["value"]);
			gboolean value; strin >> value;
			mailbox_[count_]->is_local (value);
		}

		else if (fmap["name"] == "location")
			mailbox_[count_]->location (fmap["value"]);

		else if (fmap["name"] == "hostname")
			mailbox_[count_]->hostname (fmap["value"]);

		else if (fmap["name"] == "port") {
			std::istringstream strin(fmap["value"]);
			guint value; strin >> value;
			mailbox_[count_]->port(value);
		}

		else if (fmap["name"] == "folder")
			mailbox_[count_]->folder (fmap["value"]);

		else if (fmap["name"] == "username")
			mailbox_[count_]->username (fmap["value"]);

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

		else if (fmap["name"] == "use_ssl") {
			std::istringstream strin(fmap["value"]);
			gboolean value; strin >> value;
			mailbox_[count_]->use_ssl(value);
		}

		else if (fmap["name"] == "certificate")
			mailbox_[count_]->certificate (fmap["value"]);

		else if (fmap["name"] == "polltime") {
			std::istringstream strin(fmap["value"]);
			guint value; strin >> value;
			mailbox_[count_]->polltime(value);
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
		else if (fmap["name"] == "no_clear_password") {
			std::istringstream strin(fmap["value"]);
			strin >> no_clear_password_;
		}

		else if (fmap["name"] == "sound_type") {
			std::istringstream strin(fmap["value"]);
			strin >> sound_type_;
		}

		else if (fmap["name"] == "sound_command") {
			sound_command_ = fmap["value"];
		}

		else if (fmap["name"] == "sound_file") {
			sound_file_ = fmap["value"];
			if (!g_file_test (sound_file_.c_str(), G_FILE_TEST_EXISTS))
				sound_file_ = GNUBIFF_DATADIR"/coin.wav";
		}

		else if (fmap["name"] == "sound_volume") {
			std::istringstream strin(fmap["value"]);
			strin >> sound_volume_;
		}

		else if (fmap["name"] == "check_mode") {
			std::istringstream strin(fmap["value"]);
			strin >> check_mode_;
		}

		else if (fmap["name"] == "max_mail") {
			std::istringstream strin(fmap["value"]);
			strin >> max_mail_;
		}

		else if (fmap["name"] == "mail_app") {
			mail_app_ = fmap["value"];
		}

		//
		// Popup
		//
		else if (fmap["name"] == "popup_display") {
			std::istringstream strin(fmap["value"]);
			strin >> popup_display_;
		}

		else if (fmap["name"] == "popup_time") {
			std::istringstream strin(fmap["value"]);
			strin >> popup_time_;
		}

		else if (fmap["name"] == "popup_use_geometry") {
			std::istringstream strin(fmap["value"]);
			strin >> popup_use_geometry_;
		}

		else if (fmap["name"] == "popup_geometry") {
			popup_geometry_ = fmap["value"];
		}

		else if (fmap["name"] == "popup_is_decorated") {
			std::istringstream strin(fmap["value"]);
			strin >> popup_is_decorated_;
		}

		else if (fmap["name"] == "popup_max_line") {
			std::istringstream strin(fmap["value"]);
			strin >> popup_max_line_;
		}

		else if (fmap["name"] == "popup_max_sender_size") {
			std::istringstream strin(fmap["value"]);
			strin >> popup_max_sender_size_;
		}

		else if (fmap["name"] == "popup_max_subject_size") {
			std::istringstream strin(fmap["value"]);
			strin >> popup_max_subject_size_;
		}

		else if (fmap["name"] == "popup_display_date") {
			std::istringstream strin(fmap["value"]);
			strin >> popup_display_date_;
		}

		else if (fmap["name"] == "popup_font") {
			popup_font_ = fmap["value"];
		}

		else if (fmap["name"] == "popup_font_color") {
			popup_font_color_ = fmap["value"];
		}

		//
		// Biff
		//
		else if (fmap["name"] == "biff_newmail_image") {
			biff_newmail_image_ = fmap["value"];
			if (!g_file_test (biff_newmail_image_.c_str(), G_FILE_TEST_EXISTS))
				biff_newmail_image_ = GNUBIFF_DATADIR"/tux-awake.png";
		}

		else if (fmap["name"] == "biff_nomail_image") {
			biff_nomail_image_ = fmap["value"];
			if (!g_file_test (biff_nomail_image_.c_str(), G_FILE_TEST_EXISTS))
				biff_nomail_image_ = GNUBIFF_DATADIR"/tux-sleep.png";
		}

		else if (fmap["name"] == "biff_use_newmail_image") {
			std::istringstream strin(fmap["value"]);
			strin >> biff_use_newmail_image_;
		}

		else if (fmap["name"] == "biff_use_nomail_image") {
			std::istringstream strin(fmap["value"]);
			strin >> biff_use_nomail_image_;
		}

		else if (fmap["name"] == "biff_use_newmail_text") {
			std::istringstream strin(fmap["value"]);
			strin >> biff_use_newmail_text_;
		}

		else if (fmap["name"] == "biff_use_nomail_text") {
			std::istringstream strin(fmap["value"]);
			strin >> biff_use_nomail_text_;
		}

		else if (fmap["name"] == "biff_newmail_text")
			biff_newmail_text_ = fmap["value"];

		else if (fmap["name"] == "biff_nomail_text")
			 biff_nomail_text_ = fmap["value"];

		else if (fmap["name"] == "biff_use_geometry") {
			std::istringstream strin(fmap["value"]);
			strin >> biff_use_geometry_;
		}

		else if (fmap["name"] == "biff_geometry") {
			biff_geometry_ = fmap["value"];
		}

		else if (fmap["name"] == "biff_is_decorated") {
			std::istringstream strin(fmap["value"]);
			strin >> biff_is_decorated_;
		}

		else if (fmap["name"] == "biff_font")
			biff_font_ = fmap["value"];

		else if (fmap["name"] == "biff_font_color")
			biff_font_color_ = fmap["value"];
	}
}

void
Biff::xml_error (GMarkupParseContext *context,
				 GError *error)
{
	g_warning ("%s\n", error->message);
}
