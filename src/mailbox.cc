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

#include "mailbox.h"
#include "biff.h"
#include "socket.h"
#include "ui-applet.h"
#include "ui-authentication.h"
#include "nls.h"


/**
 * "C" binding
 **/
extern "C" {
	gpointer MAILBOX_lookup (gpointer data)
	{
		MAILBOX(data)->lookup_thread ();
		return 0;
	}

	gboolean MAILBOX_watch_timeout (gpointer data)
	{
		MAILBOX(data)->watch_timeout ();
		return false;
	}

	gpointer MAILBOX_watch (gpointer data)
	{
		MAILBOX(data)->watch_thread ();
		return 0;
	}
}

guint Mailbox::uin_count_ = 1;
Authentication *Mailbox::ui_authentication_ = 0;


Mailbox::Mailbox (Biff *biff)
{
	biff_ = biff;

	if (ui_authentication_ == 0)
		ui_authentication_ = new Authentication ();


	// Default parameters
	uin_ = uin_count_++;
	protocol_ = PROTOCOL_NONE;
	gchar *text = g_strdup_printf (_("mailbox %d"), uin_);
	name_ = text;
	g_free (text);
	is_local_ = true;
	if (g_getenv ("MAIL"))
		location_ = g_getenv ("MAIL");
	if (g_getenv ("HOSTNAME"))
		hostname_ = g_getenv ("HOSTNAME");
	port_ = 110;
	folder_ = "INBOX";
	if (g_get_user_name ())
		username_ = g_get_user_name ();
	use_ssl_ = 0;
	polltime_ = 600;
	status_ = MAILBOX_UNKNOWN;

	// Internal stuff
	hidden_.clear();
	seen_.clear();
	polltag_ = 0;
	watch_mutex_ = g_mutex_new();
	object_mutex_ = g_mutex_new();
}

Mailbox::Mailbox (const Mailbox &other)
{
	biff_     = other.biff_;
	uin_      = other.uin_;
	name_     = other.name_;
	is_local_ = other.is_local_;
	location_ = other.location_;
	hostname_ = other.hostname_;
	port_     = other.port_;
	folder_   = other.folder_;
	username_ = other.username_;
	password_ = other.password_;
	polltime_ = other.polltime_;
	use_ssl_  = other.use_ssl_;
	certificate_=other.certificate_;
	protocol_ = other.protocol_;
	status_   = MAILBOX_UNKNOWN;

	// Internal stuff
	hidden_.clear();
	seen_.clear();
	polltag_ = 0;
	watch_mutex_ = g_mutex_new();
	object_mutex_ = g_mutex_new();
}

Mailbox::~Mailbox (void)
{
	watch_off();

	// We do not delete the object until we can get a grip on mutexes
	g_mutex_lock (watch_mutex_);
	g_mutex_unlock (watch_mutex_);
	g_mutex_free (watch_mutex_);
	g_mutex_lock (object_mutex_);
	g_mutex_unlock (object_mutex_);
	g_mutex_free (object_mutex_);
}

/**
 * Watch functions
 **/
void
Mailbox::watch (void)
{
	GError *err = NULL;
#ifdef DEBUG
	g_message ("[%d] Attempting to create a thread...", uin_);
#endif
	g_thread_create (MAILBOX_watch, this, FALSE, &err);
	if (err != NULL)  {
		g_warning (_("[%d] Unable to create thread: %s\n"), uin_, err->message);
		g_error_free (err);
	} 
#ifdef DEBUG
	else
		g_message ("[%d] Thread creation is ok", uin_);
#endif
}

void
Mailbox::watch_on (guint delay)
{
	g_mutex_lock (object_mutex_);
	if (polltag_ == 0) {
		if (delay)
			polltag_ = g_timeout_add (delay*1000, MAILBOX_watch_timeout, this);
		else
			polltag_ = g_timeout_add (polltime_*1000, MAILBOX_watch_timeout, this);
	}
	g_mutex_unlock (object_mutex_);
}

void Mailbox::watch_off (void) {
	g_mutex_lock (object_mutex_);
	if (polltag_ > 0)
		g_source_remove (polltag_);
	polltag_ = 0;
	g_mutex_unlock (object_mutex_);
}

void Mailbox::mark_all (void) {
	g_mutex_lock (object_mutex_);
	hidden_.clear();
	hidden_ = seen_;
	unread_.clear();
	biff_->save();
	g_mutex_unlock (object_mutex_);
}

gboolean Mailbox::watch_timeout (void) {
	g_thread_create (MAILBOX_watch, this, FALSE, 0);
	return false;
}

/**
 * This function is used within a thread so we need to take care that no other
 * thread has been started before. This is the purpose of the watch mutex
 **/
void
Mailbox::watch_thread (void) {
	if (protocol_ == PROTOCOL_NONE) {
		biff_->lookup (this);
		return;
	}

	if (!g_mutex_trylock (watch_mutex_)) {
#ifdef DEBUG
		g_message ("[%d] Cannot lock watch mutex\n", uin_);
#endif
		return;
	}

#ifdef DEBUG
	g_message ("[%d] Lock watch mutex\n", uin_);
#endif

	// Stop automatic watch
	watch_off();

	// Nobody should access status in write mode but this thread, so we're safe
	get_status();

#ifdef DEBUG
	if (status_ == MAILBOX_ERROR)	
		g_message ("[%d] MAILBOX ERROR\n", uin_);
	else if (status_ == MAILBOX_BLOCKED)
		g_message ("[%d] MAILBOX UNSECURE: BLOCKED\n", uin_);
	else if (status_ == MAILBOX_OLD)
		g_message ("[%d] MAILBOX GOT ONLY OLD MAIL\n", uin_);
	else if (status_ == MAILBOX_EMPTY)
		g_message ("[%d] MAILBOX IS EMPTY\n", uin_);
	else
		g_message ("[%d] MAILBOX GOT NEW MAIL\n", uin_);
#endif

	// Do we need to get headers ?
	if (status_ == MAILBOX_EMPTY) {
		unread_.clear();
		seen_.clear();
	}
	else if ((status_ != MAILBOX_ERROR) && (status_ != MAILBOX_OLD) && (status_ != MAILBOX_BLOCKED))
		get_header();

	gdk_threads_enter();
	biff_->applet()->process();
	gdk_threads_leave();

	gdk_threads_enter();
	biff_->applet()->update();
	gdk_threads_leave();

	g_mutex_unlock (watch_mutex_);
#ifdef DEBUG
	g_message ("[%d] Unlock watch mutex\n", uin_);
#endif
	
	// This line will force a mailbox lookup next time
	if (status_ == MAILBOX_ERROR)
		protocol_ = PROTOCOL_NONE;
}

void
Mailbox::lookup (void)
{
	GError *err = NULL;
	g_thread_create (MAILBOX_lookup, this, FALSE, &err);
	if (err != NULL)  {
		g_warning (_("Unable to create lookup thread: %s\n"), err->message);
		g_error_free (err);
	}
}

void
Mailbox::lookup_thread (void)
{


	g_mutex_lock (watch_mutex_);

	// Local mailbox
	if (is_local ()) {
		std::string location = location_;

		// Strip terminal '/' (if any)
		if (location[location.size()-1] == '/')
			location = location.substr (0, location.size()-1);
		
		// Is it a directory ?
		if (g_file_test (location.c_str(), G_FILE_TEST_IS_DIR)) {
			std::string mh_sequence = location + "/.mh_sequences";
			std::string maildir_new = location + "/new";

			if (g_file_test (mh_sequence.c_str(), G_FILE_TEST_EXISTS))
				protocol_ = PROTOCOL_MH;
			else if (location.find ("new") != std::string::npos)
				protocol_ = PROTOCOL_MAILDIR;
			else if (g_file_test (maildir_new.c_str(), G_FILE_TEST_IS_DIR))
				protocol_ = PROTOCOL_MAILDIR;
		}
		// Is it a file
		else if (g_file_test (location.c_str(), (G_FILE_TEST_EXISTS))) {
			if (location.find (".mh_sequences") != std::string::npos)
				protocol_ = PROTOCOL_MH;
			else
				protocol_ = PROTOCOL_FILE;
		}
		else
			protocol_ = PROTOCOL_NONE;
	}

	// Distant mailbox
	else {
		std::string line;
		Socket s(this);
		if (s.open (hostname_, port_, use_ssl_)) {
			// Get server greeting line
			s.read (line, true);

			if (line.find("+OK") == 0) {
				s.write ("QUIT\r\n");
				s.close();
				if (line.find ("<") != std::string::npos) {
#ifdef HAVE_CRYPTO

#ifdef HAVE_LIBSSL
					if (use_ssl_)
						protocol_ = PROTOCOL_POP3;
					else
#endif
						protocol_ = PROTOCOL_APOP;
#else
					protocol_ = PROTOCOL_POP3;
#endif
				}
				else {
				  protocol_ = PROTOCOL_POP3;
				}
			}
			else if ((line.find ("IMAP4") != std::string::npos) ||
					 (line.find ("Imap4") != std::string::npos) ||
					 (line.find ("imap4") != std::string::npos))	{
				s.write ("A001 LOGOUT\r\n");
				s.close ();
				protocol_ = PROTOCOL_IMAP4;
			}
			else
				protocol_ = PROTOCOL_NONE;
		}
		else
			protocol_ = PROTOCOL_NONE;
	}

	if (protocol_ != PROTOCOL_NONE)
		status_ = MAILBOX_EMPTY;
	
	g_mutex_unlock (watch_mutex_);
}


void
Mailbox::get_status (void)
{
	g_warning (_("Mailbox format is unknown"));
	status_ = MAILBOX_ERROR;
}

void
Mailbox::get_header (void)
{
	g_warning (_("Mailbox format is unknown"));
	status_ = MAILBOX_ERROR;
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
