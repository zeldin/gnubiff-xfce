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

#include <algorithm>
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
#include "nls.h"


// ========================================================================
//  Static features
// ========================================================================	
guint Mailbox::uin_count_ = 1;

// ========================================================================
//  base
// ========================================================================	
Mailbox::Mailbox (Biff *biff)
{
	biff_ = biff;
	listed_ = false;
	stopped_ = false;

	// Add options
	add_options (OPTGRP_MAILBOX);

	// Set session specific options
	value ("uin", uin_count_++);

	timetag_ = 0;
	mutex_ = g_mutex_new();
	monitor_mutex_ = g_mutex_new();
}

Mailbox::Mailbox (const Mailbox &other)
{
	biff_			  = other.biff_;
	add_option ((Mailbox &)other);

	status (MAILBOX_UNKNOWN);
	timetag_= 0;
	mutex_ = g_mutex_new();
	monitor_mutex_ = g_mutex_new();
}

Mailbox &
Mailbox::operator= (const Mailbox &other)
{
	if (this == &other)
		return *this;
	biff_			  = other.biff_;
	guint saved_uin = value_uint ("uin");
	add_option ((Mailbox &)other);
	value ("uin", saved_uin);
	return *this;
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

	// Is there already a timeout?
	if ((delay) && (timetag_))
		return;

	// Do we want to start now?
	if (delay)
		timetag_ = g_timeout_add (delay*1000, start_delayed_entry_point, this);
	//  or later (delay is given in seconds)?
	else
		start_delayed_entry_point (this);

#if DEBUG
	g_message ("[%d] Start fetch in %d second(s)", uin(), delay);
#endif
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
	if ((!value) || (!g_mutex_trylock (mutex_)))
		return;
	hidden_.clear();
	hidden_ = seen_;
	unread_.clear();
	biff_->save();
	g_mutex_unlock (mutex_);
}

/**
 *  This function is called when an option is changed that has the
 *  OPTFLG_CHANGE flag set.
 *
 *  @param option Pointer to the option that is changed.
 */
void 
Mailbox::option_changed (Option *option)
{
	if (!option)
		return;

	// DELAY
	if (option->name() == "delay") {
		value ("delay_minutes", ((Option_UInt *)option)->value()/60, false);
		value ("delay_seconds", ((Option_UInt *)option)->value()%60, false);
		return;
	}

	// DELAY_MINUTES, DELAY_SECONDS
	if ((option->name() == "delay_minutes")
		|| (option->name() == "delay_seconds")) {
		guint dly = value_uint("delay_minutes")*60+value_uint("delay_seconds");
		value ("delay", dly, false);
		return;
	}

	// OTHER_FOLDER, USE_OTHER_FOLDER
	if ((option->name() == "other_folder")
		|| (option->name() == "use_other_folder")) {
		if ((!value_bool ("use_other_folder"))
			|| (value_string("other_folder").size() == 0))
			value ("folder", "INBOX");
		else
			value ("folder", value_string ("other_folder"));
		return;
	}

	// OTHER_PORT, USE_OTHER_PORT, PROTOCOL, AUTHENTICATION
	if ((option->name() == "other_port") || (option->name() == "protocol")
		|| (option->name() == "use_other_port")
		|| (option->name() == "authentication")) {
		guint newport = 0;

		// Standard ports
		if (!value_bool ("use_other_port")) {
			guint auth = authentication();
			switch (protocol ()) {
			case PROTOCOL_IMAP4:
				newport = (auth == AUTH_USER_PASS) ? 143 : 993;
				break;
			case PROTOCOL_POP3:
				newport = (auth == AUTH_USER_PASS) ? 110 : 995;
				break;
			case PROTOCOL_APOP:
				newport = (auth == AUTH_APOP) ? 110 : 995;
				break;
			default:
				break;
			}
			if (auth == AUTH_AUTODETECT)
				newport = 0;
		}
		// User given port
		else
			newport = value_uint ("other_port") % 65536;

		// Set port
		value ("port", newport);
		return;
	}

	// SEEN
	if (option->name() == "seen") {
		((Option_String *)option)->get_values (hidden_);
		return;
	}

	// UIN
	if (option->name() == "uin") {
		if (value_string("name").size() == 0) {
			gchar *text = g_strdup_printf (_("mailbox %d"),
										   ((Option_UInt *)option)->value());
			value ("name", text);
			g_free (text);
		}
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
Mailbox::option_update (Option *option)
{
	if (!option)
		return;

	// SEEN
	if (option->name() == "seen") {
		((Option_String *)option)->set_values (hidden_);
		return;
	}
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
//      1.3 Is the address not a file, not a directory then Unknown
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
	g_message ("[%d] Mailbox \"%s\" type is unknown, looking up...", uin(),
			   name().c_str());
#endif

	Mailbox *mailbox = 0;

	// Local mailbox
	if (g_path_is_absolute (address().c_str()))
		mailbox=lookup_local(*this);
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
		//                  0      1       2      3     4      5
		guint    prt [6] = {port(), port(),   995,  993,   110,   143};
		gboolean ssl [6] = {true  ,  false,  true, true, false, false};

		// Port is given and authentication uses ssl so we do not try other methods
		if ((use_other_port()) && ((authentication() == AUTH_SSL)
								   || (authentication() == AUTH_CERTIFICATE)))
			for (guint i=1; i<6; i++)
				prt[i] = 0;
		// Port is given but authentication method is unknown, we only try given port with and without ssl
		else if ((use_other_port()) && ((authentication() != AUTH_AUTODETECT)))
			for (guint i=2; i<6; i++)
				prt[i] = 0;
		// Standard port is required, we do not use port()
		else if (!use_other_port()) {
			prt[0] = 0;
			prt[1] = 0;
			// SSL is required, we do not try port 110 & 143
			if ((authentication() == AUTH_SSL)
				|| (authentication() == AUTH_CERTIFICATE)) {
				prt[4] = 0;
				prt[5] = 0;
			}
			// SSL is forbidden, we do not try port 995 & 993
			else if ((authentication() == AUTH_USER_PASS)
					 || (authentication() == AUTH_APOP)) {
				prt[2] = 0;
				prt[3] = 0;
			}
		}

		guint i;
		for (i=0; i<6; i++) {
			if (stopped_) {
				g_mutex_unlock (monitor_mutex_);
				return;
			}

			

			if (prt[i] && s.open (address(), prt[i],
								   (ssl[i]==true)?AUTH_SSL:AUTH_USER_PASS, "", 5)) {
#ifdef DEBUG
				g_message ("[%d] Mailbox \"%s\", port %d opened",  uin(),
						   name().c_str(), prt[i]);
#endif
				// Get server greetings
				s.read (line, true);

				if (line.find("+OK") == 0) {
					s.write ("QUIT\r\n");
					s.close();
#ifdef HAVE_CRYPTO
					if (line.find ("<") != std::string::npos) {
						mailbox = new Apop (*this);
						mailbox->port (prt[i]);
						mailbox->authentication ((ssl[i]==true)?AUTH_SSL:AUTH_USER_PASS);
						if ((authentication() == AUTH_AUTODETECT) && !ssl[i])
							authentication (AUTH_APOP);
						break;
					}
					else
#endif
					{
					    mailbox = new Pop3 (*this);
						mailbox->port (prt[i]);
						mailbox->authentication ((ssl[i]==true)?AUTH_SSL:AUTH_USER_PASS);
						break;
					}
				}
				else if ((line.find("* OK") == 0)
						 || (line.find("* PREAUTH")== 0)) {
					s.write ("A001 LOGOUT\r\n");
					s.close ();
					mailbox = new Imap4 (*this);
					mailbox->port (prt[i]);
					mailbox->authentication ((ssl[i]==true)?AUTH_SSL:AUTH_USER_PASS);
					break;
				}
			}
		}
		port (prt[i]);
		if (authentication() == AUTH_AUTODETECT) {
			if (ssl[i])
				authentication (AUTH_SSL);
			else 
				authentication (AUTH_USER_PASS);
		}
	}

#ifdef DEBUG
	if (mailbox) {
		std::string type = value_to_string ("protocol", mailbox->protocol());
		g_message ("[%d] Ok, mailbox \"%s\" type is %s, monitoring starting in 3 seconds", uin(), name().c_str(), type.c_str());
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
	g_message ("[%d] mailbox \"%s\" type is still unknown, retrying in 3 seconds", uin(), name().c_str());
#endif

	threaded_start (3);
}


/**
 * Identification of the type of a local mailbox.
 * The type of the mailbox that belongs to the (local) address of the given
 * mailbox {\em oldmailbox} is determined. If this cannot be done, NULL will
 * be returned. Otherwise a new mailbox of this type is created, the
 * attributes of {\em oldmailbox} are copied to this mailbox. The return value
 * is a pointer to this mailbox.
 *
 * Note: This is a static function.
 *
 * @param  oldmailbox  Mailbox from which the address is taken.
 * @return             New mailbox of the correct type, or NULL if type cannot
 *                     be determined.
 */
Mailbox * 
Mailbox::lookup_local (Mailbox &oldmailbox)
{
	Mailbox *mailbox=NULL;
	const gchar *address=oldmailbox.address().c_str();
	gchar *base=g_path_get_basename(address);

	// Is it a directory?
	if (g_file_test (address, G_FILE_TEST_IS_DIR)) {
		gchar *mh_seq=g_build_filename(address,".mh_sequences",NULL);
		gchar *md_new=g_build_filename(address,"new",NULL);

		if (g_file_test (mh_seq, G_FILE_TEST_IS_REGULAR))
			mailbox = new Mh (oldmailbox);
		else if (std::string(base)=="new")
			mailbox = new Maildir (oldmailbox);
		else if (g_file_test (md_new, G_FILE_TEST_IS_DIR))
			mailbox = new Maildir (oldmailbox);

		g_free(mh_seq);
		g_free(md_new);
	}
	// Is it a file?
	else if (g_file_test (address, (G_FILE_TEST_EXISTS))) {
		if (std::string(base)==".mh_sequences")
			mailbox = new Mh (oldmailbox);
		else
			mailbox = new File (oldmailbox);
	}

	g_free(base);
	return mailbox;
}

// ================================================================================
//  Parse a mail string array
// --------------------------------------------------------------------------------
//  This function parse a mail (as a vector of string) to extract
//  sender/date/subject, to convert strings if necessary and to store the mail in
//  unread array (depending if it's spam or if it is internally marked as seen)
// ================================================================================
void Mailbox::parse (std::vector<std::string> &mail, std::string uid,
					 PartInfo *partinfo)
{
	Header h;
	gboolean status = true; // set to false if mail should not be stored
	guint len = mail.size ();
	std::string type, subtype; // MIME type
	std::map<std::string, std::string> paras; // content type parameters
	std::string encoding;

	// Parse header
	guint i;
	for (i = 0; i < len; i++) {
		// Beginning of body? (header and body are separated by an empty line)
		if (mail[i].empty())
			break;

		// Only header lines are handled now
		// Remove folding in header (see RFC 2822 2.2.2 & 2.2.3.)
 		std::string line = mail[i];
 		if (mail[i].empty() == false)
			while ((i+1 < len) && (mail[i+1].size() > 0)
				   && ((mail[i+1].at(0) == ' ') || (mail[i+1].at(0) == '\t')))
 				line += mail[++i];

		std::string line_down = ascii_strdown (line);

		// Sender
		// There should be a whitespace or a tab after "From:", so we look
		// for "From:" and get string beginning at 6
		if ((line.find ("From:") == 0) && h.sender().empty()) {
			if (line.size() > 6)
				h.sender (line.substr (6));
			else
				h.sender (_("<no sender>"));
			continue;
		}

		// Subject
		// There should a whitespace or a tab after "Subject:", so we look
		// for "Subject:" and get string beginning at 9
		if ((line.find ("Subject:") == 0) && h.subject().empty()) {
			if (line.size() > 9)
				h.subject (line.substr (9));
			else
				h.subject (_("<no subject>"));
			continue;
		}

		// Date
		// There should a whitespace or a tab after "Date:", so we look
		// for "Date:" and get string beginning at 6
		if ((line.find ("Date:") == 0) && h.date().empty()) {
			if (line.size() > 6)
				h.date (line.substr (6));
			else
				h.date (_("<no date>"));
			continue;
		}

		// Content Type
		if (line.find ("Content-Type:") == 0) {
			if (!parse_contenttype (line, type, subtype, paras)) {
				h.add_to_body (_("[Cannot parse content type header line]"));
#ifdef DEBUG
				g_message ("[%d] Cannot parse content type header line: %s",
						   uin(), line.c_str());
#endif
				continue;
			}

#ifdef DEBUG
			std::string dbg="[%d] Parsed content type header line: mimetype=";
			dbg += type + "/" + subtype;
			std::map<std::string, std::string>::iterator it = paras.begin ();
			while (it != paras.end()) {
				dbg += std::string(" (") + it->first.c_str() + " = "
				  		+ it->second.c_str() + ")";
				it++;
			}
			g_message (dbg.c_str(), uin());
#endif
			continue;
		}

		// Content Transfer Encoding (see RFC 2045 6.)
		if ((line.find ("Content-Transfer-Encoding:") == 0)
			&& (encoding.size() == 0)) {
			guint pos = 26; // size of "Content-Transfer-Encoding:"

			// Ignore whitespace
			while ((pos < line.size())
				   && ((line[pos] == ' ') || (line[pos] == '\t')))
				pos++;

			// Get Token
			if (!get_mime_token (line, encoding, pos)) {
				h.add_to_body (_("[Cannot parse content transfer encoding header line]"));
				continue;
			}
		}

		// Status
	    if ((line.find ("Status: R") == 0) && (uid.size()>0))
			status = false;
		else if (line.find ("X-Mozilla-Status: 0001") == 0)
			status = false;
		else if (line_down.find ("x-spam-flag: yes") == 0)
			status = false;
	}

	// If some information was retrieved before insert it now
	if (partinfo) {
		// Content type parameters
		if (partinfo->parameters_.size() > 0)
			paras = partinfo->parameters_;

		// MIME type and subtype
		if (partinfo->type_.size() > 0)
			type = partinfo->type_;
		if (partinfo->subtype_.size() > 0)
			subtype = partinfo->subtype_;

		// Encoding
		if (partinfo->encoding_.size() > 0)
			encoding = partinfo->encoding_;
	}

	// Charset
	if (h.charset().empty() && (paras.find("charset") != paras.end()))
		h.charset (paras["charset"]);

	// Set default values if no value can be obtained from the mail (see
	// RFC 2045)
	if (h.charset().empty())
		h.charset ("us-ascii");
	if ((type.size() == 0) || (subtype.size() == 0)) {
		type = "text";
		subtype = "plain";
	}
	if (encoding.size() == 0)
		encoding = "7bit";

	// Decode mail body (may change length of mail)
	decode_body (mail, encoding, i);
	len = mail.size ();

	// Content type: text/plain
	if ((type == "text") && (subtype == "plain")) {
		// Get mail body
		guint j = 0;
		while ((j < biff_->value_uint("popup_body_lines")) && (++i < len)) {
			if (j++)
				h.add_to_body ("\n");
			h.add_to_body (mail[i]);
		}
		if ((j == biff_->value_uint ("popup_body_lines")) && (i+2 < len))
			h.add_to_body ("\n...");
	}
	else {
		gchar *tmp = g_strdup_printf (_("[This mail hasn't a supported content type: \"%s/%s\"]"), type.c_str(), subtype.c_str());
		if (tmp)
			h.add_to_body (std::string (tmp));
		g_free (tmp);
	}

	// Store mail depending on status
	if (status)
		// Pines trick
		if (h.subject().find("DON'T DELETE THIS MESSAGE -- FOLDER INTERNAL DATA") == std::string::npos) {
			// Ok, at this point mail is
			//  - not a spam
			//  - has not been read (R or 0001 status flag)
			//  - is not a special header (see above)
			// so we have to decide what to do with it because we may have already displayed it
			// and maybe it has been gnubifficaly marked as "seen".
			//
			h.mailid (uid);
			if (hidden_.find (h.mailid()) == hidden_.end ()) {
				h.position (new_unread_.size() + 1);
				h.mailbox_uin (value_uint ("uin"));
				new_unread_[h.mailid()] = h;
			}
			new_seen_.insert (h.mailid());
#ifdef DEBUG
			g_message ("[%d] Parsed mail with id \"%s\"", uin(),
					   h.mailid().c_str ());
#endif
		}
}

/** 
 *  Parses the "Content-Type:" line in a mail header. Only if true is
 *  returned {\em type}, {\em subtype} and {\em map} will have defined
 *  values. The parameters will be added to the map {\em map}, this map
 *  will not be cleared before. As attributes are always case insensitive
 *  they will be normalized to be in lower case. See also RFC 2045 5.1. for
 *  the syntax of the content type header field.
 *
 *  @param  line    Line from the mail header (folding already removed)
 *  @param  type    Here the type of the mail's content type will be returned
 *  @param  subtype Here the subtype of the mail's content type will be
 *                  returned
 *  @param  map     Here the parameters will be returned as pairs (attribute,
 *                  value).
 *  @return         Boolean indicating success.
 */
gboolean 
Mailbox::parse_contenttype (std::string line, std::string &type,
							std::string &subtype,
							std::map<std::string, std::string> &map)
{
	// Non alphanumeric characters allowed in tokens
	const static std::string token_ok = "!#$%&'*+-._`{|}~";

	// Test, if we really have a content type line
	if (line.find ("Content-Type:") != 0)
		return false;
	line = line.substr (13);

	// Initialize
	type = "";
	subtype = "";

	guint len = line.size(), pos = 0;
	// Ignore whitespace
	while ((pos < len) && ((line[pos] == ' ') || (line[pos] == '\t')))
		pos++;

	// Get type
	if (!get_mime_token (line, type, pos))
		return false;

	// Separator '/' between type and subtype
	if ((pos >= len) || (line[pos++] != '/'))
		return false;

	// Get subtype
	if (!get_mime_token (line, subtype, pos))
		return false;

	// Get parameters
	while (true) {
		// Ignore whitespace
		while ((pos < len) && ((line[pos] == ' ') || (line[pos] == '\t')))
			pos++;

		// End of line?
		if (pos >= len)
			break;

		// There must be a ';' now
		if (line[pos++] != ';')
			return false;

		// Ignore whitespace
		while ((pos < len) && ((line[pos] == ' ') || (line[pos] == '\t')))
			pos++;

		// Get attribute
		std::string attr;
		while ((pos < len) && ((token_ok.find(line[pos]) != std::string::npos)
							   || (g_ascii_isalnum(line[pos]))))
			attr += line[pos++];
		if (attr.size() == 0)
			return false;
		attr = ascii_strdown (attr);

		// Ignore whitespace
		while ((pos < len) && ((line[pos] == ' ') || (line[pos] == '\t')))
			pos++;

		// Separator '=' between attribute and value
		if ((pos >= len) || (line[pos++] != '='))
			return false;

		// Ignore whitespace
		while ((pos < len) && ((line[pos] == ' ') || (line[pos] == '\t')))
			pos++;

		// Get value; can be token or quoted string
		if (pos >= len)
			return false;
		std::string value;
		// Quoted string
		if (line[pos] == '"') {
			if (!get_quotedstring (line, value, pos))
				return false;
		}
		// Token
		else
			if (!get_mime_token (line, value, pos, false))
				return false;

		// Insert pair (attribute, value) into map
		map[attr] = value;
	}

	return true;
}

/**
 *  This function is to be called if the mail has been displayed, so status
 *  can be changed from MAILBOX_NEW to MAILBOX_OLD. If the status isn't
 *  MAILBOX_NEW nothing is done.
 */
void 
Mailbox::mail_displayed (void)
{
	if (status() == MAILBOX_NEW)
		status (MAILBOX_OLD);
}

/**
 *  Determine the new status of the mailbox and keep the values obtained in the
 *  last update.
 */
void 
Mailbox::update_mailbox_status (void)
{
	// Determine new mailbox status
	if (status() != MAILBOX_CHECK)
		return;
	if (new_unread_.size() == 0)
		status (MAILBOX_EMPTY);
	else if (unread_.size() < new_unread_.size())
		status (MAILBOX_NEW);
	else if (!std::includes (unread_.begin(), unread_.end(),
							 new_unread_.begin(), new_unread_.end(),
							 less_pair_first()))		 
		status ((unread_ == new_unread_) ? MAILBOX_OLD : MAILBOX_NEW);
	else
		status (MAILBOX_OLD);

	// Remove mails from hidden that are no longer needed
	std::set<std::string> new_hidden;
	std::set_intersection (hidden_.begin(), hidden_.end(),
						   new_seen_.begin(), new_seen_.end(),
						   std::insert_iterator<std::set<std::string> > (new_hidden, new_hidden.begin()));

	// Save obtained values
	g_mutex_lock (mutex_);
	unread_ = new_unread_;
	seen_ = new_seen_;
	hidden_ = new_hidden;
	g_mutex_unlock (mutex_);

	// Clear sets for next update
	new_unread_.clear ();
	new_seen_.clear ();
}

/**
 * Start checking for new messages.
 */
void 
Mailbox::start_checking (void)
{
	// Set mailbox status
	status (MAILBOX_CHECK);

	// Fetch mails and update status
	fetch ();
	update_mailbox_status ();
}

/**
 * Decide whether a mail has to be fetched and parsed. If the mail is already
 * known it is inserted into the new_unread_ map. So it can be displayed later.
 *
 * @param     mailid Gnubiff mail identifier of the mail in question
 * @return           False if the mail has to be fetched and parsed, true
 *                   otherwise
 */
gboolean 
Mailbox::new_mail(std::string &mailid)
{
	// We have now seen this mail
	new_seen_.insert (mailid);

	// Mail shall not be displayed? -> no need to fetch and parse it
	if (hidden_.find (mailid) != hidden_.end ()) {
#ifdef DEBUG
		g_message ("[%d] Ignore mail with id \"%s\"", uin(), mailid.c_str ());
#endif
		return true;
	}

	// Mail unknown?
	if (unread_.find (mailid) == unread_.end ())
		return false;

	// Insert known mail into new unread mail map
	new_unread_[mailid] = unread_[mailid];
	new_unread_[mailid].position (new_unread_.size());

#ifdef DEBUG
	g_message ("[%d] Already read mail with id \"%s\"", uin(), mailid.c_str());
#endif
	return true;
}

/**
 *  Search the mailbox for the mail with id {\em mailid}.
 *
 *  @param  mailid  Gnubiff mail identifier of the mail to find
 *  @param  mail    Here the header of the found mail is returned. If no mail
 *                  with id {\em mailid} exists, {\em mail} remains unchanged.
 *  @returns        Boolean indicating if a mail exists or not.
 */
gboolean 
Mailbox::find_mail (std::string mailid, Header &mail)
{
	gboolean ok = false;

	g_mutex_lock (mutex_);
	if ((ok = (unread_.find (mailid) != unread_.end ())))
		mail = unread_[mailid];
	g_mutex_unlock (mutex_);

	return ok;
}
