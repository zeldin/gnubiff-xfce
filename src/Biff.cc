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
#include <map>
#include <sstream>
#include <sys/stat.h>
#include <utime.h>
#include "Biff.h"
#include "AppletGTK.h"
#ifdef USE_GNOME
#  include "AppletGNOME.h"
#endif
#include "Mailbox.h"
#include "File.h"
#include "Mh.h"
#include "Maildir.h"
#include "Imap4.h"
#include "Pop3.h"
#include "Apop.h"
#include "Popup.h"



// ===================================================================
// = Callbacks "C" binding ===========================================
// ===================================================================
extern "C" {
	/** "C" binding for the XML parser (see glib doc) */
	void BIFF_xml_start_element (GMarkupParseContext *context, const gchar *element_name,const gchar **attribute_names, const gchar **attribute_values, gpointer data, GError **error) 
	{((Biff *) data)->xml_start_element (context, element_name, attribute_names, attribute_values, error);};
	/** "C" binding for the XML parser (see glib doc) */
	void BIFF_xml_error (GMarkupParseContext *context, GError *error, gpointer data)
	{((Biff *) error)->xml_error (context, error);};
}

// ===================================================================
// = Static attributes ===============================================
// ===================================================================


// ===================================================================
// = Biff ===========================================================
/**
   Create a new Biff object by setting default parameters, then
   loading the configuration file from filename if it exists. If the
   given configuration file is empty, then a default one
   ($HOME/.gnubiffrc) is created from default parameters.
   @param mode is the mode to use (GNOME/GTK) for GNOME version
   @param filename is the configuration file to use
*/
// ===================================================================
Biff::Biff (gint mode, std::string filename) {
	// Create the crypt table for encryption and decryption of password
	// This table is dependent on the "/bin" directory change time It
	// should be sufficiently different from one system to the other so
	// even if someone stoled the file, he won't be able to decrypt it
	// on his system without this information (which is easily
	// accessible anyway).
	// Note that this time should be pretty stable over time.
	_table = "";

#ifdef USE_PASSWORD
	_table = PASSWORD_STRING;
	// Add something in case provided string is not long enough
	_table += "FEDCBA9876543210";

	// Now, we remove duplicated characters
	std::string buffer;
	for (guint i=0; i<_table.size(); i++)
		if (buffer.find(_table[i]) == std::string::npos)
			buffer += _table[i];
	_table = buffer;
#endif

	// Interface
	_mode = mode;

	// Setup
	_setup = new Setup (this, GNUBIFF_DATADIR"/Setup.glade");
	_setup->create ();

	// Applet
#ifdef USE_GNOME
	if (_mode == GNOME_MODE)
		_applet = new AppletGNOME (this, GNUBIFF_DATADIR"/AppletGNOME.glade");
	else
		_applet = new AppletGTK (this, GNUBIFF_DATADIR"/AppletGTK.glade");
#else
	_applet = new AppletGTK (this, GNUBIFF_DATADIR"/AppletGTK.glade");
#endif
	_applet->create();

	// Popup
	_popup = new Popup (this,  GNUBIFF_DATADIR"/Popup.glade");
	_popup->create();

	// Mailbox (at least one)
	_mailbox.push_back (new File (this));

	_poptime = 5;
	_sound_type = SOUND_FILE;
	_sound_command = "play %s -v %v";
	_sound_file = REAL_DATADIR"/sounds/gnubiff/coin.wav";
	_sound_volume = 100;
	_state = STATE_RUNNING;
	_display_popup = true;
	_mail_app = "xemacs";
	_mail_image = GNUBIFF_DATADIR"/tux-awake.png";
	_nomail_image = GNUBIFF_DATADIR"/tux-sleep.png";
	_hide_nomail_image = false;
	_hide_newmail_image = false;
	_hide_nomail_text = false;
	_hide_newmail_text = false;
	_nomail_text = "%d";
	_newmail_text = "%d";
	_display_date = true;
	_max_sender_size = 50;
	_max_subject_size = 50;
	_max_collected_mail = 50;
	_popup_x_sign = SIGN_MINUS;
	_popup_x = 0;
	_popup_y_sign = SIGN_PLUS;
	_popup_y = 0;
	_popup_decorated = FALSE;
	_popup_no_geometry = FALSE;
	_biff_x_sign = SIGN_PLUS;
	_biff_x = 0;
	_biff_y_sign = SIGN_PLUS;
	_biff_y = 0;
	_biff_decorated = FALSE;
	_biff_no_geometry = FALSE;
	_max_header_display = 50;
	_popup_font = "Sans 10"; 
	_applet_font = "Sans 8";
	_popup_font_color = "Black"; 
	_popup_back_color = "White"; 
	_applet_font_color = "Blue";


	// Do we have a valid filename ?
	if (!filename.empty())
		_filename = filename;
	// no ? => use default one and try to load it (if it exists)
	else 
		_filename = std::string (g_get_home_dir ()) + std::string ("/.gnubiffrc");

	// In Gnome mode the applet has to be docked before setting up configuration information
	// can be read
	if (_mode == GNOME_MODE) {
		_mailbox.clear();
		_mailbox.push_back (new File (this));
		return;
	}

	// Does configuration file exist ?
	std::ifstream file;
	file.open (_filename.c_str());
	if (file.is_open()) {
		file.close();
		load();
	}
	else
		g_error (_("Configuration file (%s) not found !"), _filename.c_str());


	// Update GUI
	_setup->update();
}


// ===================================================================
// = ~Biff ==========================================================
// ===================================================================
Biff::~Biff (void) {
}

// ===================================================================
// = save ============================================================
/**
   This function tries to save the current biff
   @return false if save failed, true else
*/
// ===================================================================
gboolean Biff::save (void) {
	// Open file
	std::ofstream file;
	file.open (_filename.c_str());
	if (!file.is_open())
		return false;
  
	// Header
	file << "<?xml version=\"1.0\"?>" << std::endl;
	file << "<configuration-file>"<< std::endl;

	// Mailboxes
	for (unsigned int i=0; i< _mailbox.size(); i++) {
		file << "  <mailbox>" << std::endl;
		file << "    <parameter name=\"protocol\"         value=\"" << _mailbox[i]->protocol() << "\"/>" << std::endl;
		file << "    <parameter name=\"name\"             value=\"" << _mailbox[i]->name() << "\"/>" << std::endl;
		file << "    <parameter name=\"port\"             value=\"" << _mailbox[i]->port() << "\"/>" << std::endl;
		file << "    <parameter name=\"use_ssl\"          value=\"" << _mailbox[i]->use_ssl() << "\"/>" << std::endl;
		file << "    <parameter name=\"address\"          value=\"" << _mailbox[i]->address() << "\"/>" << std::endl;
		file << "    <parameter name=\"folder\"           value=\"" << _mailbox[i]->folder() << "\"/>" << std::endl;
		file << "    <parameter name=\"user\"             value=\"" << _mailbox[i]->user() << "\"/>" << std::endl;

		// Here we are: we save password.
		// pop3 and imap4 protocols requires password in clear so we have to save password
		// in clear within configuration file. No need to say this is higly unsecure if
		// somebody looks at the file.
		// So we have to take some measures:
		//   1. The file is made readable by owner only.
		//   2. password is "crypted" so it's not directly human readable.
		// Of course, these measures won't prevent a determined person to break in but will
		// at least prevent "ordinary" people to be able to steal password easily.
#ifdef USE_PASSWORD
		file << "    <parameter name=\"password\"         value=\"";
		for (guint j=0; j<_mailbox[i]->password().size(); j++)
			file << _table[_mailbox[i]->password()[j]/16] << _table[_mailbox[i]->password()[j]%16];
		file << "\"/>" << std::endl;
#else
		file << "    <parameter name=\"password\"         value=\"" << "" << "\"/>" << std::endl;
#endif
		file << "    <parameter name=\"certificate\"      value=\"" << _mailbox[i]->certificate() << "\"/>" << std::endl;
		file << "    <parameter name=\"polltime\"         value=\"" << _mailbox[i]->polltime() << "\"/>" << std::endl;
		file << "    <parameter name=\"seen\"             value=\"";
		for (unsigned int j=0; j<_mailbox[i]->hiddens(); j++)
			file << _mailbox[i]->hidden(j) << " ";
		file << "\"/>" << std::endl;
		file << "  </mailbox>" << std::endl;
	}

	// Options
	file << "  <options>" << std::endl;
	file << "    <parameter name=\"poptime\"          value=\"" << _poptime << "\"/>" << std::endl;
	file << "    <parameter name=\"sound_type\"       value=\"" << _sound_type << "\"/>" << std::endl;
	file << "    <parameter name=\"sound_command\"    value=\"" << _sound_command << "\"/>" << std::endl;
	file << "    <parameter name=\"sound_file\"       value=\"" << _sound_file << "\"/>" << std::endl;
	file << "    <parameter name=\"sound_volume\"     value=\"" << _sound_volume << "\"/>" << std::endl;
	file << "    <parameter name=\"state\"            value=\"" << _state << "\"/>" << std::endl;
	file << "    <parameter name=\"max_collected_mail\" value=\"" << _max_collected_mail << "\"/>" << std::endl;
  
	//  => Advanced
	file << "    <advanced>" << std::endl;
	file << "      <parameter name=\"display_popup\"      value=\"" << _display_popup << "\"/>" << std::endl;
	file << "      <parameter name=\"mail_app\"           value=\"" << _mail_app << "\"/>" << std::endl;
	file << "    </advanced>" << std::endl;

	// End Options
	file << "  </options>" << std::endl;


	// Layout
	file << "  <layout>" << std::endl;
	file << "    <parameter name=\"mail_image\"         value=\"" << _mail_image << "\"/>" << std::endl;
	file << "    <parameter name=\"nomail_image\"       value=\"" << _nomail_image << "\"/>" << std::endl;
	file << "    <parameter name=\"hide_nomail_image\"  value=\"" << _hide_nomail_image << "\"/>" << std::endl;
	file << "    <parameter name=\"hide_newmail_image\" value=\"" << _hide_newmail_image << "\"/>" << std::endl;
	file << "    <parameter name=\"hide_nomail_text\"   value=\"" << _hide_nomail_text << "\"/>" << std::endl;
	file << "    <parameter name=\"hide_newmail_text\"  value=\"" << _hide_newmail_text << "\"/>" << std::endl;
	file << "    <parameter name=\"nomail_text\"        value=\"" << _nomail_text << "\"/>" << std::endl;
	file << "    <parameter name=\"newmail_text\"       value=\"" << _newmail_text << "\"/>" << std::endl;
	file << "    <parameter name=\"display_date\"       value=\"" << _display_date << "\"/>" << std::endl;
	file << "    <parameter name=\"max_sender_size\"    value=\"" << _max_sender_size << "\"/>" << std::endl;
	file << "    <parameter name=\"max_subject_size\"   value=\"" << _max_subject_size << "\"/>" << std::endl;

  
	//  => Geometry
	file << "    <geometry>" << std::endl;
	file << "      <parameter name=\"popup_x_sign\"       value=\"" << _popup_x_sign << "\"/>" << std::endl;
	file << "      <parameter name=\"popup_x\"            value=\"" << _popup_x << "\"/>" << std::endl;
	file << "      <parameter name=\"popup_y_sign\"       value=\"" << _popup_y_sign << "\"/>" << std::endl;
	file << "      <parameter name=\"popup_y\"            value=\"" << _popup_y << "\"/>" << std::endl;
	file << "      <parameter name=\"popup_decorated\"    value=\"" << _popup_decorated << "\"/>" << std::endl;
	file << "      <parameter name=\"popup_no_geometry\"  value=\"" << _popup_no_geometry << "\"/>" << std::endl;
	file << "      <parameter name=\"biff_x_sign\"        value=\"" << _biff_x_sign << "\"/>" << std::endl;
	file << "      <parameter name=\"biff_x\"             value=\"" << _biff_x << "\"/>" << std::endl;
	file << "      <parameter name=\"biff_y_sign\"        value=\"" << _biff_y_sign << "\"/>" << std::endl;
	file << "      <parameter name=\"biff_y\"             value=\"" << _biff_y << "\"/>" << std::endl;
	file << "      <parameter name=\"biff_decorated\"     value=\"" << _biff_decorated << "\"/>" << std::endl;
	file << "      <parameter name=\"biff_no_geometry\"   value=\"" << _biff_no_geometry << "\"/>" << std::endl;
	file << "      <parameter name=\"max_header_display\" value=\"" << _max_header_display << "\"/>" << std::endl;
	file << "    </geometry>" << std::endl;


	//  => Font
	file << "    <font>" << std::endl;
	file << "      <parameter name=\"popup_font\"         value=\"" << _popup_font << "\"/>" << std::endl;
	file << "      <parameter name=\"applet_font\"        value=\"" << _applet_font << "\"/>" << std::endl;
	file << "      <parameter name=\"popup_font_color\"   value=\"" << _popup_font_color << "\"/>" << std::endl;
	file << "      <parameter name=\"popup_back_color\"   value=\"" << _popup_back_color << "\"/>" << std::endl;
	file << "      <parameter name=\"applet_font_color\"  value=\"" << _applet_font_color << "\"/>" << std::endl;
	file << "    </font>" << std::endl;


	// End Layout
	file << "  </layout>" << std::endl;

	// End Header
	file << "</configuration-file>"<< std::endl;

	// Close the file
	file.close ();

	// Restrict file permission
	std::string command = std::string("chmod 600 ") + _filename;
	system (command.c_str());
  
	return true;
}


// ===================================================================
// = load ============================================================
/**
   This function tries to load a biff configuration file. The XML
   parsing is done via glib tiny xml parser.
   @return false if load failed, true else
*/
// ===================================================================
gboolean Biff::load (void) {
	_mailbox.clear();
	_counter = -1;

	// Open the file
	std::ifstream file;
	std::string line;
	file.open (_filename.c_str());
	if (!file.is_open()) {
		_mailbox.push_back (new File (this));
		g_warning (_("Cannot open your configuration file (%s)"), _filename.c_str());
		return false;
	}

	// Instantiate a new xml parser
	GMarkupParser parser;
	parser.start_element = BIFF_xml_start_element;
	parser.end_element = 0;
	parser.text = 0;
	parser.passthrough = 0;
	parser.error = BIFF_xml_error;
	GMarkupParseContext *context = g_markup_parse_context_new (&parser, GMarkupParseFlags (0), this, 0);

	// Parse the file
	gboolean status = TRUE;
	while ((!getline(file, line).eof()) && (status))
		status = g_markup_parse_context_parse (context, line.c_str(), line.size(), 0);

	if (_mailbox.size() == 0) {
		g_warning (_("Found no mailbox definition in your .gnubiffrc"));
		_mailbox.push_back (new File (this));
	}

	// Close the file
	file.close ();
	return true;
}


// ===================================================================
// = xml_start_element ===============================================
/**
   This function is called by g_markup_parse_context_parse at the
   start of a new xml element. See glib documentation for further
   information.
*/
// ===================================================================
void Biff::xml_start_element (GMarkupParseContext *context,
							  const gchar *element_name,
							  const gchar **attribute_names,
							  const gchar **attribute_values,
							  GError **error) {
	if (std::string (element_name) == "mailbox") {
		_counter++;
	}
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
				_mailbox.push_back (new File (this));
				break;
			case PROTOCOL_MH:
				_mailbox.push_back (new Mh (this));
				break;
			case PROTOCOL_MAILDIR:
				_mailbox.push_back (new Maildir (this));
				break;
			case PROTOCOL_IMAP4:
				_mailbox.push_back (new Imap4 (this));
				break;
			case PROTOCOL_POP3:
				_mailbox.push_back (new Pop3 (this));
				break;
			case PROTOCOL_APOP:
				_mailbox.push_back (new Apop (this));
				break;
			}
		}

		// Mailbox
		else if (fmap["name"] == "name")
			_mailbox[_counter]->name() = fmap["value"];
		else if (fmap["name"] == "port") {
			std::istringstream strin(fmap["value"]);
			strin >> _mailbox[_counter]->port();
		}
		else if (fmap["name"] == "use_ssl") {
			std::istringstream strin(fmap["value"]);
			strin >> _mailbox[_counter]->use_ssl();
		}
		else if (fmap["name"] == "address")
			_mailbox[_counter]->address() = fmap["value"];
		else if (fmap["name"] == "folder")
			_mailbox[_counter]->folder() = fmap["value"];
		else if (fmap["name"] == "user")
			_mailbox[_counter]->user() = fmap["value"];
		else if (fmap["name"] == "password") {
			_mailbox[_counter]->password() = "";
			std::string tmp = fmap["value"];
			for (gint i=0; i<gint(tmp.size())-1; i+=2) {
				char c = 0;
				guint j;
				for (j=0; j<16; j++) {
					if (_table [j] == tmp[i])
						c += j*16;
					if (_table [j] == tmp[i+1])
						c += j;
				}
				_mailbox[_counter]->password() += c;
			}
		}
		else if (fmap["name"] == "certificate")
			_mailbox[_counter]->certificate() = fmap["value"];
		else if (fmap["name"] == "polltime") {
			std::istringstream strin(fmap["value"]);
			strin >> _mailbox[_counter]->polltime();
		}
		else if (fmap["name"] == "seen") {
			std::istringstream strin (fmap["value"]);
			_mailbox[_counter]->hidden().clear();
			guint mailid;
			while (strin >> mailid)
				_mailbox[_counter]->hidden().push_back (mailid);
		}



		// Options
		else if (fmap["name"] == "poptime") {
			std::istringstream strin(fmap["value"]);
			strin >> _poptime;
		}
		else if (fmap["name"] == "sound_type") {
			std::istringstream strin(fmap["value"]);
			strin >> _sound_type;
		}
		else if (fmap["name"] == "sound_volume") {
			std::istringstream strin(fmap["value"]);
			strin >> _sound_volume;
		}
		else if (fmap["name"] == "sound_command") {
			_sound_command = fmap["value"];
		}
		else if (fmap["name"] == "sound_file") {
			_sound_file = fmap["value"];
			if (!g_file_test (_sound_file.c_str(), G_FILE_TEST_EXISTS))
				_sound_file = REAL_DATADIR"/sounds/gnubiff/coin.wav"; 
		}
		else if (fmap["name"] == "state") {
			std::istringstream strin(fmap["value"]);
			strin >> _state;
		}
		else if (fmap["name"] == "max_collected_mail") {
			std::istringstream strin(fmap["value"]);
			strin >> _max_collected_mail;
		}

		// Advanced
		else if (fmap["name"] == "display_popup") {
			std::istringstream strin(fmap["value"]);
			strin >> _display_popup;
		}
		else if (fmap["name"] == "mail_app")
			_mail_app = fmap["value"];

		// Layout
		else if (fmap["name"] == "mail_image") {
			_mail_image = fmap["value"];
			if (!g_file_test (_mail_image.c_str(), G_FILE_TEST_EXISTS))
				_mail_image = GNUBIFF_DATADIR"/tux-awake.png";
		}
		else if (fmap["name"] == "nomail_image") {
			_nomail_image = fmap["value"];
			if (!g_file_test (_nomail_image.c_str(), G_FILE_TEST_EXISTS))
				_nomail_image = GNUBIFF_DATADIR"/tux-sleep.png";
		}
		else if (fmap["name"] == "hide_nomail_image") {
			std::istringstream strin(fmap["value"]);
			strin >> _hide_nomail_image;
		}
		else if (fmap["name"] == "hide_newmail_image") {
			std::istringstream strin(fmap["value"]);
			strin >> _hide_newmail_image;
		}
		else if (fmap["name"] == "hide_nomail_text") {
			std::istringstream strin(fmap["value"]);
			strin >> _hide_nomail_text;
		}
		else if (fmap["name"] == "hide_newmail_text") {
			std::istringstream strin(fmap["value"]);
			strin >> _hide_newmail_text;
		}
		else if (fmap["name"] == "nomail_text")
			 _nomail_text = fmap["value"];
		else if (fmap["name"] == "newmail_text")
			 _newmail_text = fmap["value"];
		else if (fmap["name"] == "display_date") {
			std::istringstream strin(fmap["value"]);
			strin >> _display_date;
		}
		else if (fmap["name"] == "max_sender_size") {
			std::istringstream strin(fmap["value"]);
			strin >> _max_sender_size;
		}
		else if (fmap["name"] == "max_subject_size") {
			std::istringstream strin(fmap["value"]);
			strin >> _max_subject_size;
		}



		// Geometry
		else if (fmap["name"] == "popup_x_sign") {
			std::istringstream strin(fmap["value"]);
			strin >> _popup_x_sign;
		}
		else if (fmap["name"] == "popup_x") {
			std::istringstream strin(fmap["value"]);
			strin >> _popup_x;
		}
		else if (fmap["name"] == "popup_y_sign") {
			std::istringstream strin(fmap["value"]);
			strin >> _popup_y_sign;
		}
		else if (fmap["name"] == "popup_y") {
			std::istringstream strin(fmap["value"]);
			strin >> _popup_y;
		}
		else if (fmap["name"] == "popup_decorated") {
			std::istringstream strin(fmap["value"]);
			strin >> _popup_decorated;
		}
		else if (fmap["name"] == "popup_no_geometry") {
			std::istringstream strin(fmap["value"]);
			strin >> _popup_no_geometry;
		}
		else if (fmap["name"] == "biff_x_sign") {
			std::istringstream strin(fmap["value"]);
			strin >> _biff_x_sign;
		}
		else if (fmap["name"] == "biff_x") {
			std::istringstream strin(fmap["value"]);
			strin >> _biff_x;
		}
		else if (fmap["name"] == "biff_y_sign") {
			std::istringstream strin(fmap["value"]);
			strin >> _biff_y_sign;
		}
		else if (fmap["name"] == "biff_y") {
			std::istringstream strin(fmap["value"]);
			strin >> _biff_y;
		}
		else if (fmap["name"] == "biff_decorated") {
			std::istringstream strin(fmap["value"]);
			strin >> _biff_decorated;
		}
		else if (fmap["name"] == "biff_no_geometry") {
			std::istringstream strin(fmap["value"]);
			strin >> _biff_no_geometry;
		}
		else if (fmap["name"] == "max_header_display") {
			std::istringstream strin(fmap["value"]);
			strin >> _max_header_display;
		}



		// Font
		else if (fmap["name"] == "popup_font")
			_popup_font = fmap["value"];
		else if (fmap["name"] == "applet_font")
			_applet_font = fmap["value"];
		else if (fmap["name"] == "popup_font_color")
			_popup_font_color = fmap["value"];
		else if (fmap["name"] == "popup_back_color")
			_popup_back_color = fmap["value"];
		else if (fmap["name"] == "applet_font_color")
			_applet_font_color = fmap["value"];
	}
}


// ===================================================================
// = xml_error =======================================================
/**
   This function is called by g_markup_parse_context_parse if error
   occured while parsing. See glib documentation for further
   information.
*/
// ===================================================================
void Biff::xml_error (GMarkupParseContext *context, GError *error) {
	g_warning ("%s\n", error->message);
}

