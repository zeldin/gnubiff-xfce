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

#include "mailbox.h"
#include "file.h"
#include "maildir.h"
#include "mh.h"
#include "imap4.h"
#include "pop3.h"
#include "apop.h"
#include "biff.h"
#include "socket.h"
#include "ui-authentication.h"


// ========================================================================
//  Static features
// ========================================================================	
guint Mailbox::uin_count_ = 1;
Authentication *Mailbox::ui_auth_ = 0;


// ========================================================================
//  base
// ========================================================================	
Mailbox::Mailbox (Biff *biff)
{
	biff_ = biff;
	listed_ = false;
	stopped_ = false;
	if (ui_auth_ == 0)
		ui_auth_ = new Authentication ();

	// Default parameters
	uin_ = uin_count_++;
	protocol_ = PROTOCOL_NONE;
	gchar *text = g_strdup_printf (_("mailbox %d"), uin_);
	name_ = text;
	g_free (text);
	if (g_getenv ("MAIL"))
		address_ = g_getenv ("MAIL");
	else if (g_getenv ("HOSTNAME"))
		address_ = g_getenv ("HOSTNAME");
	if (g_get_user_name ())
		username_ = g_get_user_name ();
	password_ = "";
	authentication_ = AUTH_AUTODETECT;
	port_ = 0;
	folder_ = "INBOX";
	certificate_ = "";
	delay_ = 180;
	use_other_folder_ = false;
	other_folder_ = "";
	use_other_port_ = false;
	other_port_ = 995;

	status_ = MAILBOX_UNKNOWN;
	timetag_ = 0;
	hidden_.clear();
	seen_.clear();
	mutex_ = g_mutex_new();
	monitor_mutex_ = g_mutex_new();
}

Mailbox::Mailbox (const Mailbox &other)
{
	biff_			= other.biff_;
	uin_			= other.uin_;
	name_			= other.name_;
	protocol_		= other.protocol_;
	authentication_ = other.authentication_;
	address_		= other.address_;
	username_		= other.username_;
	password_		= other.password_;
	port_			= other.port_;
	folder_			= other.folder_;
	certificate_	= other.certificate_;
	delay_			= other.delay_;
	use_other_folder_= other.use_other_folder_;
	other_folder_	= other.other_folder_;
	use_other_port_	= other.use_other_port_;
	other_port_		= other.other_port_;

	status_ = MAILBOX_UNKNOWN;
	timetag_= 0;
	hidden_.clear();
	seen_.clear();
	mutex_ = g_mutex_new();
	monitor_mutex_ = g_mutex_new();
}

Mailbox::~Mailbox (void)
{
	g_mutex_lock (mutex_);
	g_mutex_unlock (mutex_);
	g_mutex_free (mutex_);
	g_mutex_lock (monitor_mutex_);
	g_mutex_unlock (monitor_mutex_);
	g_mutex_free (monitor_mutex_);
}

// ========================================================================
//  main
// ========================================================================	
void
Mailbox::threaded_start (guint delay)
{
	stopped_ = false;

	// Is there already a timeout ?
	if ((delay) && (timetag_))
		return;

	// Do we want to start now ?
	if (delay)
		timetag_ = g_timeout_add (delay*1000, start_delayed_entry_point, this);
	//  or later (delay is given in seconds) ?
	else
		start_delayed_entry_point (this);
}

gboolean
Mailbox::start_delayed_entry_point (gpointer data)
{
	GError *err = NULL;
	g_thread_create ((GThreadFunc) start_entry_point, data, FALSE, &err);
	if (err != NULL)  {
		g_warning (_("[%d] Unable to create thread: %s"), MAILBOX(data)->uin(), err->message);
		g_error_free (err);
	}
	MAILBOX(data)->timetag (0);
	return false;
}

void
Mailbox::start_entry_point (gpointer data)
{
	MAILBOX(data)->start();
}
void
Mailbox::start (void)
{
	// Since class "Mailbox" is virtual, any monitoring requires first
	// to autodetect mailbox type. Once it's done, "this" mailbox is
	// destroyed so we cannot go any further past this point.
	lookup();
}


void
Mailbox::stop (void)
{
	stopped_ = true;
	if (timetag_) {
		g_source_remove (timetag_);
		timetag_ = 0;
	}
}

void
Mailbox::fetch (void)
{
	// nothing to do, this mailbox is virtual
}

void
Mailbox::read (gboolean value)
{
	if (!g_mutex_trylock (mutex_))
		return;
	if (value == true) {
		hidden_.clear();
		hidden_ = seen_;
		unread_.clear();
		biff_->save();
	}
	g_mutex_unlock (mutex_);
}


// ================================================================================
//  lookup function to try to guess mailbox status
// --------------------------------------------------------------------------------
//  There are several hints that help us detect a mailbox format:
//
//   1. Does address begins with a '/' ? (File, Mh or Maildir)
//
//      1.1 Is the address a directory ? (Mh or Maildir)
//          1.2.1 If there is a 'new' file within then Maildir
//                                                else Mh
//      1.2 Is the address a file ? (File or Mh)
//          1.2.1 If last address component is '.mh_sequences' then Mh
//                                                             else File
//
//      1.3 Is the address not a file, not a directory then Unknwonw
//
//
//   2. Else (Pop3, Apop or Imap4)
//
//      2.1 Does server answer '+OK' (Pop3 or Apop)
//          2.1.1 If there is angle bracket in server greetings then Apop
//                                                              else Pop3
//      2.2 If there is an 'Imap4' in server greetings then Imap4
//                                                     else Unknown
//
// ================================================================================
//
// FIXME: stop lookup when displaying preferences 
//        -> need to interrupt the for loop one way or the other
void
Mailbox::lookup (void)
{
	if (!g_mutex_trylock (monitor_mutex_))
		return;

#ifdef DEBUG
	g_message ("[%d] Mailbox \"%s\" type is unknown, looking up...",  uin_, name_.c_str());
#endif

	Mailbox *mailbox = 0;

	// Local mailbox
	if (address_[0] == '/') {
		std::string address = address_;

		// Strip terminal '/' (if any)
		if (address[address.size()-1] == '/')
			address = address.substr (0, address.size()-1);

		// Is it a directory ?
		if (g_file_test (address.c_str(), G_FILE_TEST_IS_DIR)) {
			std::string mh_sequence = address + "/.mh_sequences";
			std::string maildir_new = address + "/new";

			if (g_file_test (mh_sequence.c_str(), G_FILE_TEST_EXISTS))
				mailbox = new Mh (*this);
			else if (address.find ("new") != std::string::npos)
				mailbox = new Maildir (*this);
			else if (g_file_test (maildir_new.c_str(), G_FILE_TEST_IS_DIR))
				mailbox = new Maildir (*this);
		}
		// Is it a file
		else if (g_file_test (address.c_str(), (G_FILE_TEST_EXISTS))) {
			if (address.find (".mh_sequences") != std::string::npos)
				mailbox = new Mh (*this);
			else
				mailbox = new File (*this);
		}
	}

	// Distant mailbox
	else {
		std::string line;
		Socket s(this);

		// Ok, at this stage, either the port is given or we have to try
		// 4 standard ports (pop3:110, imap4:143, spop3:995, simap4:993)
		// Also, if auth is autodetect we have to try with or without ssl.
		// So, port is organized as follows:
		//  0: given port, ssl
		//  1: given port, no ssl
		//  2: 995, ssl
		//  3: 993, ssl
		//  4: 110, no ssl
		//  5: 143, no ssl
		// Any null port means do not try
		//                  0      1       2     3     4      5
		guint    port[6] = {port_, port_,  995,  993,  110,   143};
		gboolean ssl [6] = {true,  false,  true, true, false, false};

		// Port is given and authentication uses ssl so we do not try other methods
		if ((use_other_port_) && ((authentication_ == AUTH_SSL) || (authentication_ == AUTH_CERTIFICATE)))
			for (guint i=1; i<6; i++)
				port[i] = 0;
		// Port is given but authentication method is unknown, we only try given port with and without ssl
		else if ((use_other_port_) && ((authentication_ != AUTH_AUTODETECT)))
			for (guint i=2; i<6; i++)
				port[i] = 0;
		// Standard port is required, we do not use port_
		else if (!use_other_port_) {
			port[0] = 0;
			port[1] = 0;
			// SSL is required, we do not try port 110 & 143
			if ((authentication_ == AUTH_SSL) || (authentication_ == AUTH_CERTIFICATE)) {
				port[4] = 0;
				port[5] = 0;
			}
			// SSL is forbidden, we do not try port 995 & 993
			else if ((authentication_ == AUTH_USER_PASS) || (authentication_ == AUTH_APOP)) {
				port[2] = 0;
				port[3] = 0;
			}
		}

		guint i;
		for (i=0; i<6; i++) {
			if (stopped_) {
				g_mutex_unlock (monitor_mutex_);
				return;
			}

			

			if (port[i] && s.open (address_, port[i], (ssl[i]==true)?AUTH_SSL:AUTH_USER_PASS, "", 5)) {
#ifdef DEBUG
	            g_message ("[%d] Mailbox \"%s\", port %d opened",  uin_, name_.c_str(), port[i]);
#endif
				// Get server greetings
				s.read (line, true);

				if (line.find("+OK") == 0) {
					s.write ("QUIT\r\n");
					s.close();
					if (line.find ("<") != std::string::npos) {
#ifdef HAVE_CRYPTO
						mailbox = new Apop (*this);
						mailbox->port (port[i]);
						mailbox->authentication ((ssl[i]==true)?AUTH_SSL:AUTH_USER_PASS);
						if ((authentication_ == AUTH_AUTODETECT) && !ssl[i])
							authentication_ = AUTH_APOP;
#else
					    mailbox = new Pop3 (*this);
						mailbox->port (port[i]);
						mailbox->authentication ((ssl[i]==true)?AUTH_SSL:AUTH_USER_PASS);
#endif
						break;
					}
					else {
					    mailbox = new Pop3 (*this);
						mailbox->port (port[i]);
						mailbox->authentication ((ssl[i]==true)?AUTH_SSL:AUTH_USER_PASS);
						break;
					}
				}
				else if ((line.find ("IMAP4") != std::string::npos) ||
						 (line.find ("Imap4") != std::string::npos) ||
						 (line.find ("imap4") != std::string::npos))	{
					s.write ("A001 LOGOUT\r\n");
					s.close ();
					mailbox = new Imap4 (*this);
					mailbox->port (port[i]);
					mailbox->authentication ((ssl[i]==true)?AUTH_SSL:AUTH_USER_PASS);
					break;
				}
			}
		}
		port_ = port[i];
		if (authentication_ == AUTH_AUTODETECT) {
			if (ssl[i])
				authentication_ = AUTH_SSL;
			else 
				authentication_ = AUTH_USER_PASS;
		}
	}

#ifdef DEBUG
	if (mailbox) {
		std::string type;
		switch (mailbox->protocol()) {
		case PROTOCOL_FILE:		type = "file";		break;
		case PROTOCOL_MH:		type = "mh";		break;
		case PROTOCOL_MAILDIR:	type = "maildir";	break;
		case PROTOCOL_POP3:		type = "pop3";		break;
		case PROTOCOL_APOP:		type = "apop";		break;
		case PROTOCOL_IMAP4:	type = "imap4";		break;
		}
		g_message ("[%d] Ok, mailbox \"%s\" type is %s, monitoring starting in 3 seconds", uin_, name_.c_str(), type.c_str());
	}
#endif

	// After replace, "this" is destroyed so we must return immediately
	if (mailbox) {
		g_mutex_unlock (monitor_mutex_);
		biff_->replace (this, mailbox);
		return;
	}

	g_mutex_unlock (monitor_mutex_);

#ifdef DEBUG
	g_message ("[%d] mailbox \"%s\" type is still unknown, retrying in 3 seconds", uin_, name_.c_str());
#endif

	threaded_start (3);
}

// ================================================================================
//  Parse a mail string array
// --------------------------------------------------------------------------------
//  This function parse a mail (as a vector of string) to extract
//  sender/date/subject, to convert strings if necessary and to store the mail in
//  unread array (depending if it's spam or if it is internally marked as seen)
// ================================================================================
void Mailbox::parse (std::vector<std::string> &mail, int status)
{
	header h;
	h.status = status;

	for (guint i=0; i<mail.size(); i++) {
		gchar *buffer = g_ascii_strdown (mail[i].c_str(), -1);
		std::string line = buffer;
		g_free (buffer);

		// Sender
		// There should be a whitespace or a tab after "From:", so we look
		// for "From:" and get string beginning at 6
		if ((mail[i].find ("From:") == 0) && h.sender.empty()) {
			if (mail[i].size() > 6)
				h.sender = mail[i].substr (6);
			else
				h.sender = _("<no sender>");
		}

		// Subject
		// There should a whitespace or a tab after "Subject:", so we look
		// for "Subject:" and get string beginning at 9
		else if ((mail[i].find ("Subject:") == 0) && h.subject.empty()) {
			if (mail[i].size() > 9)
				h.subject = mail[i].substr (9);
			else
				h.subject = _("<no subject>");
		}

		// Date
		// There should a whitespace or a tab after "Date:", so we look
		// for "Date:" and get string beginning at 6
		else if ((mail[i].find ("Date:") == 0) && h.date.empty()) {
			if (mail[i].size() > 6)
				h.date = mail[i].substr (6);
			else
				h.date = _("<no date>");
		}

		// Charset
		// FIXME: Currently, this code does not handle the case where a
		// charset is coded over two or more lines. Question: is is a big
		// problem ? (is it allowed anyway ?)
		else if ((line.find ("charset=") != std::string::npos) && h.charset.empty()) {
			// +8 is size of "charset="+1
			//  (we need that because find will return start of "charset=")
			std::string charset = line.substr (int(line.find ("charset="))+8);
			// First we remove any leading '"'
			if (charset[0] == '\"')
				charset = charset.substr(1);

			// Then we wait for the end of charset
			for (guint j=0; j<charset.size(); j++) {
				if ((charset[j+1] == ';')  || (charset[j+1] == '\"') ||
					(charset[j+1] == '\n') || (charset[j+1] == '\t') ||
					(charset[j+1] == ' ')  || (charset[j+1] == '\0')) {
					h.charset = charset.substr (0, j+1);
					break;
				}
			}
		}
		// Status
		else if ((mail[i].find ("Status: R") == 0) && h.status == -1)
			h.status = MAIL_READ;
		else if (mail[i].find ("X-Mozilla-Status: 0001") == 0)
			h.status = MAIL_READ;
		else if (line.find ("x-spam-flag: yes") != std::string::npos) 
			h.status = MAIL_READ;
		else if ((mail[i].empty()) && h.body.empty()) {
			guint j = 0;
			do {
				h.body += mail[i++] + std::string("\n");
				j++;
			} while ((j<10) && (i < mail.size()));
			if (j == 10)
				h.body += std::string("...");
		}
	}

	// Store mail depending on status
	if ((h.status == MAIL_UNREAD) || (h.status == -1))
		// Pines trick
		if (h.subject.find("DON'T DELETE THIS MESSAGE -- FOLDER INTERNAL DATA") == std::string::npos) {
			// Ok, at this point mail is
			//  - not a spam
			//  - has not been read (R or 0001 status flag)
			//  - is not a special header (see above)
			// so we have to decide what to do with it because we may have already displayed it
			// and maybe it has been gnubifficaly marked as "seen".
			//
			guint mailid = g_str_hash (h.sender.c_str()) ^ g_str_hash (h.subject.c_str()) ^ g_str_hash (h.date.c_str());
			guint j;
			for (j=0; j<hidden_.size(); j++)
				if (hidden_[j] == mailid)
					break;
			if (j >= hidden_.size())
				new_unread_.push_back(h);
			new_seen_.push_back (mailid);
		}
}
