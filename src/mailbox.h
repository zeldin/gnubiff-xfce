// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2006 Nicolas Rougier, 2004-2006 Robert Sowada
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
// Author(s)     : Nicolas Rougier, Robert Sowada
// Short         : 
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#ifndef __MAILBOX_H__
#define __MAILBOX_H__

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif
#include <functional>
#include <glib.h>
#include <map>
#include <regex.h>
#include <set>
#include <string>
#include "biff.h"
#include "decoding.h"
#include "header.h"
#include "socket.h"
#include "support.h"

/** This struct is needed (when using STL algorithms for
 *  std::map<std::string,Header>) for comparisons of
 *  std::pair<std::string,Header> objects. We cannot compare headers so the
 *  default compare function is not useful for us. */
struct less_pair_first : public std::binary_function<std::pair<std::string,Header>,std::pair<std::string,Header>, bool> {
	bool operator()(std::pair<std::string,Header> x,
					std::pair<std::string,Header> y) const {
	  return x.first < y.first;
	}
};

#define MAILBOX(x)					((Mailbox *)(x))

/**
 * Generic mailbox intended as base for implementing mailboxes for a specific
 * protocol. 
 */
class Mailbox : public Decoding, public Gnubiff_Options, public Support {

protected:
	// ========================================================================
	//  internal stuff
	// ========================================================================
	static guint				uin_count_;			// unique identifier number count
	class Biff *				biff_;				// biff owner
	GMutex *					mutex_;				// mutex for thread read access
	GMutex *					monitor_mutex_;		// mutex for monitor access
	guint						timetag_;			// tag for delayed start timeout
	gboolean					listed_;			// flag for updating mailboxes in preferences
	gboolean					stopped_;			// flag for stopping mailbox monitor while looking up

	void option_changed (Option *option);
	void option_update (Option *option);
	gboolean parse_contenttype (std::string line, class PartInfo &partinfo);

	/// Mail headers of mails that have not been read yet
	std::map<std::string, Header> unread_;

	/** Mail headers of mails (of the the present update) that have not been
	 *  read yet. These headers will be transfered to Mailbox::unread_ once
	 *  the updated is completed successfully. */
	std::map<std::string, Header> new_unread_;

	/// Set of gnubiff mail ids of those mails that won't be displayed
	std::set<std::string>		hidden_;

	/** Set of gnubiff mail ids of those mails that have already been seen by
	 *  gnubiff during the last update */
	std::set<std::string>		seen_;

	/** Set of gnubiff mail ids of those mails that have already been seen by
	 *  gnubiff during the present update. These ids will be transfered to
	 *  Mailbox::seen_ once the update is completed successfully. */
	std::set<std::string>		new_seen_;

public:
	// ========================================================================
	//  base
	// ========================================================================
	Mailbox (class Biff *biff);
	Mailbox (const Mailbox &other);
	Mailbox &operator= (const Mailbox &other);
	virtual ~Mailbox (void);

	// ========================================================================
	//  exceptions
	// ========================================================================
	/** Generic exception for mailboxes. This only serves as a base for more
	 *  more specific exceptions. */
	class mailbox_err : public std::exception
	{
	private:
		/** If {\em mailboxerror_} is true this exception will imply a mailbox
		 *  error status when handled, otherwise lookup will be terminated but
		 *  mailbox status will be left untouched. */
		gboolean mailboxerror_;
	public:
		/** Constructor. 
		 *
		 * @param mailboxerror Whether this exception should imply a mailbox
		 *                     error status or not. The default is true. */
		mailbox_err (gboolean mailboxerror=true) {mailboxerror_=mailboxerror;}
		/// Access function to mailbox_err::mailboxerror_. 
		gboolean is_mailboxerror() {return mailboxerror_;}
	};

	// ========================================================================
	//  main
	// ========================================================================
	virtual void threaded_start (guint delay = 0);				// start monitoring in a new thread
	static gboolean start_delayed_entry_point (gpointer data);	// start thread timeout entry point
	static void start_entry_point (gpointer data);				// start thread entry point

	virtual void start (void);						// start method (to be overidden)
	virtual void stop (void);						// stop method (to be overidden)
	virtual void fetch (void);						// fetch headers (if any)
	void mark_messages_as_read (void);
	void lookup (void);								// try to guess mailbox format
	static Mailbox *lookup_local(Mailbox &);        // try to guess mailbox format for a local mailbox
	gboolean new_mail (std::string &);
	void update_mailbox_status (void);
	void start_checking (void);
	void set_status_mailbox_error (void);
	void mail_displayed (void);
	void parse (std::vector<std::string> &mail,
				std::string uid = std::string(""),
				class PartInfo *pi = NULL, class Header *hh = NULL,
				guint pos = 0, gboolean status = true);
	guint static standard_port (guint protocol, guint auth,
								gboolean strict = true);

	// ========================================================================
	//  main -- messages
	// ========================================================================

	void get_message_headers (std::vector<Header *> &headers,
							  gboolean use_max_num = false, guint max_num = 0,
							  gboolean empty = false);

	// ========================================================================
	//  filtering
	// ========================================================================

private:
	/// Compiled regular expressions for filtering messages by header lines
	std::vector<regex_t *> filter_regex_;
	/// Options for each regular expression
	std::vector<std::string> filter_opts_;

	gboolean filter_add (std::vector<std::string> &regex_strs);
	void filter_free (void);
	gboolean filter_match_line (std::string line, gboolean &status);

public:
	gboolean filter_create (void);

	// ========================================================================
	//  access
	// ========================================================================

	gboolean find_mail (std::string mailid, Header &mail);

	/// Access function to Mailbox::biff_
	class Biff *biff (void)								{return biff_;}
	/// Access function to mailbox option "name"
	const std::string name (void)						{return value_string ("name");}
	/// Access function to mailbox option "name"
	void name (const std::string val)					{value ("name", val);}
	/// Access function to mailbox option "protocol"
	const guint protocol (void)							{return value_uint ("protocol");}
	/// Access function to mailbox option "protocol"
	void protocol (const guint val)						{value ("protocol", val);}
	/// Access function to mailbox option "authentication"
	const guint authentication (void)					{return value_uint ("authentication");}
	/// Access function to mailbox option "authentication"
	void authentication (const guint val)				{value ("authentication", val);}	
	/// Access function to mailbox option "address"
	const std::string address (void)					{return value_string ("address");}
	/// Access function to mailbox option "address"
	void address (const std::string val)				{value ("address", val);}
	/// Access function to mailbox option "username"
	const std::string username (void)					{return value_string ("username");}
	/// Access function to mailbox option "username"
	void username (const std::string val)				{value ("username", val);}
	/// Access function to mailbox option "password"
	const std::string password (void)					{return value_string ("password");}
	/// Access function to mailbox option "password"
	void password (const std::string val)				{value ("password", val);}
	/// Access function to mailbox option "port"
	const guint port (void)								{return value_uint ("port");}
	/// Access function to mailbox option "port"
	void port (const guint val)							{value ("port", val);}
	/// Access function to mailbox option "folder"
	const std::string folder (void)						{return value_string ("folder");}
	/// Access function to mailbox option "folder"
	void folder (const std::string val)					{value ("folder",val);}
	/// Access function to mailbox option "certificate"
	const std::string certificate (void)				{return value_string ("certificate");}
	/// Access function to mailbox option "certificate"
	void certificate (const std::string val)			{value ("certificate", val);}
	/// Access function to mailbox option "delay"
	  const guint delay (void)							{return value_uint ("delay");}
	/// Access function to mailbox option "delay"
	void delay (const guint val)						{value ("delay", val);}
	/// Access function to mailbox option "use_idle"
	const gboolean use_idle (void)						{return value_bool ("use_idle");}
	/// Access function to mailbox option "use_idle"
	void use_idle (gboolean val)						{value ("use_idle", val);}
	/// Access function to mailbox option "use_other_folder"
	const gboolean use_other_folder (void)				{return value_bool ("use_other_folder");}
	/// Access function to mailbox option "use_other_folder"
	void use_other_folder (const gboolean val)			{value ("use_other_folder", val);}
	/// Access function to mailbox option "other_folder"
	const std::string other_folder (void)				{return value_string ("other_folder");}
	/// Access function to mailbox option "other_folder"
	void other_folder (const std::string val)			{value ("other_folder", val);}
	/// Access function to mailbox option "use_other_port"
	const gboolean use_other_port (void)				{return value_bool ("use_other_port");}
	/// Access function to mailbox option "use_other_port"
	void use_other_port (const gboolean val)			{value ("use_other_port", val);}
	/// Access function to mailbox option "other_port"
	const guint other_port (void)						{return value_uint ("other_port");}
	/// Access function to mailbox option "other_port"
	void other_port (const guint val)					{value ("other_port", val);}
	/// Access function to mailbox option "status"
	const guint status (void) 							{return value_uint ("status");}
	/// Access function to mailbox option "status"
	void status (const guint status)					{value ("status", status);}

	const gboolean listed (void)						{return listed_;}
	void listed (gboolean value)						{listed_ = value;}

	/// Access function to mailbox option "uin"
	const guint uin (void)								{return value_uint ("uin");}
#ifdef DEBUG
	/// Access function to Mailbox::uin_count_ (only when debugging is enabled)
	static guint uin_count (void)						{return uin_count_;}
#endif

	const guint timetag (void)							{return timetag_;}
	void timetag (guint value)							{timetag_ = value;}
	
	
	/// Access function to Mailbox::unread_
	std::map<std::string, Header> &unread (void)		{return unread_;}
	/// Number of unread mails
	guint unreads (void) {
		g_mutex_lock (mutex_);
		guint s = unread_.size();
		g_mutex_unlock (mutex_);
		return s;
	}
	/// Access function to Mailbox::hidden_
	std::set<std::string> &hidden (void)				{return hidden_;}
	/// Number of mails that won't be displayed
	guint hiddens (void)								{return hidden_.size();}
	/// Access function to Mailbox::seen_
	std::set<std::string> &seen (void)					{return seen_;}
};

/**
 * Information about one part of a (possible) multi-part mail. If the mail
 * consists only of one part the information is valid for the whole mail.
 */
class PartInfo
{
 public:
	/** Part identifier as needed for the IMAP command FETCH (see
	 *  RFC 3501 6:4:5). This is the part of the mail that will be displayed
	 *  by gnubiff (if possible). */
	std::string part_;
	/** Error message. When the mail will be displayed the mail's body is
	 *  substituted by this message. */
	std::string error_;
	/// MIME type
	std::string type_;
	/// MIME subtype
	std::string subtype_;
	/** Encoding of this part. Currently supported encodings are 7bit, 8bit
	 *  and quoted-printable. Encodings yet to be supported are binary and
	 *  base64. */
	std::string encoding_;
	/// Parameters as pairs (attribute, value)
	std::map<std::string, std::string> parameters_;
	/// Size of this part in bytes
	gint size_;
};

#endif
