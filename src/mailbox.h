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

#ifndef __MAILBOX_H__
#define __MAILBOX_H__

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif
#include <glib.h>
#include <map>
#include <set>
#include <string>
#include "biff.h"
#include "decoding.h"
#include "socket.h"


/**
 * Constant definition
 **/
const guint	PROTOCOL_NONE			=	0;
const guint	PROTOCOL_FILE			=	1;
const guint	PROTOCOL_POP3			=	2;
const guint	PROTOCOL_IMAP4			=	3;
const guint	PROTOCOL_MAILDIR		=	4;
const guint	PROTOCOL_MH				=	5;
const guint	PROTOCOL_APOP			=	6;

const gint	MAILBOX_ERROR			=	0;
const gint	MAILBOX_EMPTY			=	1;
const gint	MAILBOX_OLD				=	2;
const gint	MAILBOX_NEW				=	3;
const gint	MAILBOX_CHECK			=	4;
const gint	MAILBOX_STOP			=	5;
const gint	MAILBOX_UNKNOWN			=	6;

const gint	MAIL_UNREAD				=	0;
const gint	MAIL_READ				=	1;


// ========================================================================
//  Header type definition
// ========================================================================
typedef struct _header {
	std::string	sender;
	std::string	subject;
	std::string	date;
	std::string	body;
	std::string	charset;
	gint		status;

	/**
	 *  This is a (hopefully) unique identifier for the mail. If supported by
	 *  the protocol this will be the unique id of the mail that is provided
	 *  by the server (this is the case for POP3 and IMAP4). Otherwise
	 *  gnubiff creates an own identifier.
	 *
	 *  Remark: This identifier must not contain whitespace characters!
	 *
	 *  @see The mail identifier is calculated by the method
	 *       header::setmailid().
	 */
	std::string mailid_;

	struct _header &operator = (const struct _header &other)
	{
		if (this != &other) {
			sender = other.sender;
			subject = other.subject;
			date = other.date;
			body = other.body;
			charset = other.charset;
			status = other.status;
			mailid_ = other.mailid_;
		}
		return *this;
	}

	bool operator == (const struct _header &other) const
	{
		if ((sender  == other.sender)  && (subject == other.subject) &&
			(date    == other.date)    && (body    == other.body)    &&
			(charset == other.charset) && (status  == other.status)  &&
			(mailid_ == other.mailid_))
			return true;
		else
			return false;
	}

	/**
	 *  Setting the gnubiff mail identifier for this mail header. If a unique
	 *  identifier {\em uid} is provided it is taken. Otherwise (i.e.
	 *  {\em uid} is an empty string) it is created by concatenating hash
	 *  values of the sender, subject and date.
	 *
	 *  @param uid Unique identifier for the mail as provided by the protocol
	 *             (POP3 and IMAP4). The default is the empty string.
	 */
	void setmailid (std::string uid = std::string(""))
	{
		if (uid.size () > 0)
			mailid_ = uid;
		else {
			std::stringstream ss;
			ss << g_str_hash (sender.c_str());
			ss << g_str_hash (subject.c_str());
			ss << g_str_hash (date.c_str());
			mailid_ = ss.str ();
		}
	}
} header;


#define MAILBOX(x)					((Mailbox *)(x))

/**
 * Generic mailbox intended as base for implementing mailboxes for a specific
 * protocol. 
 */
class Mailbox : public Decoding {

protected:
	// ========================================================================
	//  "real" configuration
	// ========================================================================
	std::string					name_;				// displayed name
	guint						protocol_;			// protocol
	gint						authentication_;	// authentication method
	std::string					address_;			// address of mailbox
	std::string					username_;			// username
	std::string					password_;			// password
	guint						port_;				// port
	std::string					folder_;			// mailbox folder
	std::string					certificate_;		// certificate file
	guint						delay_;				// delay between mail check (apop & pop3 only)
	/** Use IDLE command if server supports it. This is usually a good idea.
	 *  But if multiple clients connect to the same mailbox this can lead to
	 *  connection errors (depending on the internal server configuration).
	 *  For users encountering this problem it is better not to use idling but
	 *  use polling instead. */
	gboolean                    use_idle_;

	// ========================================================================
	//  "convenience" configuration
	// ========================================================================	
	gboolean					use_other_folder_;	// whether to use other port
	std::string					other_folder_;		// other mailbox folder
	gboolean					use_other_port_;	// whether to use other port
	guint						other_port_;		// other port

	// ========================================================================
	//  internal stuff
	// ========================================================================
	guint						uin_;				// unique identifier number
	static guint				uin_count_;			// unique identifier number count
	class Biff *				biff_;				// biff owner
	gint 						status_;			// status of the mailbox
	GMutex *					mutex_;				// mutex for thread read access
	GMutex *					monitor_mutex_;		// mutex for monitor access
	guint						timetag_;			// tag for delayed start timeout
	gboolean					listed_;			// flag for updating mailboxes in preferences
	gboolean					stopped_;			// flag for stopping mailbox monitor while looking up

	/// Mail headers of mails that have not been read yet
	std::map<std::string, header> unread_;
	/** Mail headers of mails (of the the present update) that have not been
	 *  read yet. These headers will be transfered to Mailbox::unread_ once
	 *  the updated is completed successfully. */
	std::map<std::string, header> new_unread_;
	/// Set of gnubiff mail ids of those mails that won't be displayed
	std::set<std::string>		hidden_;
	/** Set of gnubiff mail ids of those mails that have already been seen by
	 *  gnubiff during the last update */
	std::set<std::string>		seen_;
	/** Set of gnubiff mail ids of those mails that have already been seen by
	 *  gnubiff during the present update. These ids will be transfered to
	 *  Mailbox::seen_ once the update is completed successfully. */
	std::set<std::string>		new_seen_;
	/** This vector contains the gnubiff mail ids of all those mails that will
	 *  be displayed (in the opposite order). */
	std::vector<std::string>    mails_to_be_displayed_;
	/** Into this vector the gnubiff mail ids of all those mails that will
	 *  be displayed (in the opposite order) when the current update is
	 *  finished are inserted. */
	std::vector<std::string>    new_mails_to_be_displayed_;

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
	void read (gboolean value=true);				// mark/unmark mailbox as read
	void lookup (void);								// try to guess mailbox format
	static Mailbox *lookup_local(Mailbox &);        // try to guess mailbox format for a local mailbox
	gboolean new_mail (std::string &);
	void start_checking (void);
	void parse (std::vector<std::string> &mail,		// parse a mail 
				int status = -1, std::string uid = std::string(""));

	// ========================================================================
	//  access
	// ========================================================================
	const std::string name (void)						{return name_;}
	void name (const std::string value)					{name_ = value;}

	const guint protocol (void)							{return protocol_;}
	void protocol (const guint value)					{protocol_ = value;}

	const guint authentication(void)					{return authentication_;}
	void authentication (const guint value)				{authentication_ = value;}	

	const std::string address (void)					{return address_;}
	void address (const std::string value)				{address_ = value;}

	const std::string username (void)					{return username_;}
	void username (const std::string value)				{username_ = value;}

	const std::string password (void)					{return password_;}
	void password (const std::string value)				{password_ = value;}

	const guint port (void)								{return port_;}
	void port (const guint value)						{port_ = value;}

	const std::string folder (void)						{return folder_;}
	void folder (const std::string value)				{folder_ = value;}

	const std::string certificate (void)				{return certificate_;}
	void certificate (const std::string value)			{certificate_ = value;}

	const guint delay (void)							{return delay_;}
	void delay (const guint value)						{delay_ = value;}

	/// Access function to Mailbox::use_idle_
	const gboolean use_idle (void)						{return use_idle_;}
	/// Access function to Mailbox::use_idle_
	void use_idle (gboolean value)						{use_idle_ = value;}

	const gboolean use_other_folder (void)				{return use_other_folder_;}
	void use_other_folder (const gboolean value)		{use_other_folder_ = value;}

	const std::string other_folder (void)				{return other_folder_;}
	void other_folder (const std::string value)			{other_folder_ = value;}

	const gboolean use_other_port (void)				{return use_other_port_;}
	void use_other_port (const gboolean value)			{use_other_port_ = value;}

	const guint other_port (void)						{return other_port_;}
	void other_port (const guint value)					{other_port_ = value;}

	const gint status (void) 							{return status_;}
	void status (const gint status)						{status_ = status;}

	const gboolean listed (void)						{return listed_;}
	void listed (gboolean value)						{listed_ = value;}

	const guint uin (void)								{return uin_;}

	const guint timetag (void)							{return timetag_;}
	void timetag (guint value)							{timetag_ = value;}
	
	
	/// Access function to Mailbox::unread_
	std::map<std::string, header> &unread (void)		{return unread_;}
	/// Number of unread mails
	guint unreads (void) {
		g_mutex_lock (mutex_);
		guint s = unread_.size();
		g_mutex_unlock (mutex_);
		return s;
	}
	/// Access function to Mailbox::mails_to_be_displayed_
	std::vector<std::string> &mails_to_be_displayed (void) {
		g_mutex_lock (mutex_);
		std::vector<std::string> &tmp=mails_to_be_displayed_;
		g_mutex_unlock (mutex_);
		return tmp;
	}
	/// Access function to Mailbox::hidden_
	std::set<std::string> &hidden (void)				{return hidden_;}
	/// Number of mails that won't be displayed
	guint hiddens (void)								{return hidden_.size();}
	/// Access function to Mailbox::seen_
	std::set<std::string> &seen (void)					{return seen_;}
};

/**
 * Maximum number of lines to be read from the mail body. This value should be
 * greater than the value of the lines displayed;-) (see "src/ui-popup.cc").
 */
const gint bodyLinesToBeRead_=12;

/**
 * In some situations we need to read a certain number of lines from the
 * network to get the line we want. Unfortunately this number may vary in
 * reality because of the following reasons:
 * \begin{itemize}
 *    \item The server sends information or warning messages (see RFC 3501
 *          7.1.1 and 7.1.2)
 *    \item There is no limit for the response
 *    \item There exist different extensions to the protocols
 *    \item Not all servers implement protocols correctly
 *    \item There is a DoS attack
 * \end{itemize}
 * To prevent being DoS attacked we need to set a limit of additional lines
 * that are being read. This is done by the following constant.
 */
const gint preventDoS_additionalLines_=16;

/**
 * To prevent being DoS attacked (see above): This constant is used when the
 * server is expected to need a lot of time to complete a command (the IMAP
 * "IDLE" command for example) but may send information and warning messages
 * before completion. This constant gives the maximum number of such messages
 * before gnubiff assumes a DoS attack. 
 */
const gint preventDoS_ignoreinfo_=32;

/**
 * To prevent being DoS attacked (see above): Maximum number of header lines
 * being read. This is currently only used for POP3.
 */
const gint preventDoS_headerLines_=2048;

/**
 * To prevent being DoS attacked (see above): Limit for length of a read line.
 * \begin{itemize}
 *    \item SMTP: maximum line length is 1001 (see RFC 2821 4.5.3.1)
 *    \item IMAP: no maximum line length
 *    \item POP3: maximum response line length is 512 (see RFC 1939 3.)
 * \end{itemize}
 */
const gint preventDoS_lineLength_=16384;

/**
 * To prevent being DoS attacked (see above): For several IMAP4 commands the
 * server's response can consist of more than one line. This constant gives the
 * maximum number of lines being read (additional to the first line) before
 * gnubiff suspects a DoS attack.
 * 
 * Currently this constant is used when reading the response to the following
 * commands: FETCH (BODYSTRUCTURE).
 */
const gint preventDoS_imap4_multilineResponse_=8;

#endif
