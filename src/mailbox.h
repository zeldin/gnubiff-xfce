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

	struct _header &operator = (const struct _header &other)
	{
		if (this != &other) {
			sender = other.sender;
			subject = other.subject;
			date = other.date;
			body = other.body;
			charset = other.charset;
			status = other.status;
		}
		return *this;
	}

	bool operator == (const struct _header &other) const
	{
		if ((sender  == other.sender)  && (subject == other.subject) &&
			(date    == other.date)    && (body    == other.body)    &&
			(charset == other.charset) && (status  == other.status))
			return true;
		else
			return false;
	}
} header;


#define MAILBOX(x)					((Mailbox *)(x))


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

	std::vector<header>			unread_;			// collected unread mail
	std::vector<header>			new_unread_;		// collected unread mail (tmp buffer)
	std::vector<guint>			hidden_;			// mails that won't be displayed
	std::vector<guint>			seen_;				// mails already seen   
	std::vector<guint>			new_seen_;			// mails already seen (tmp buffer)

	static class Authentication *ui_auth_;			// ui to get username & password
	static GStaticMutex			ui_auth_mutex_;		// Lock to avoid conflicts
	template<class T> static gboolean contains_new (std::vector<T> newlist, std::vector<T> oldlist); // Comparing newlist to oldlist for new elements

public:
	// ========================================================================
	//  base
	// ========================================================================	
	Mailbox (class Biff *biff);
	Mailbox (const Mailbox &other);
	Mailbox &operator= (const Mailbox &other);
	virtual ~Mailbox (void);

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
	void parse (std::vector<std::string> &mail,		// parse a mail 
				int status = -1);

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
	
	

	std::vector<header> &unread (void)					{return unread_;}
	header &unread (int i)								{return unread_[i];}
	guint unreads (void) {
		g_mutex_lock (mutex_);
		guint s = unread_.size();
		g_mutex_unlock (mutex_);
		return s;
	}
	std::vector<guint> &hidden (void)					{return hidden_;}
	guint &hidden (int i)								{return hidden_[i];}
	guint hiddens (void)								{return hidden_.size();}
};

/**
 * Determine if an element is contained in {\em newlist} that is not in
 * {\em oldlist}.  We use this function to determine if {\em newlist} contains
 * an element that is not in {\em oldlist}.  This method is necessary for
 * checking for new unique mail messages from a prior check since mail count
 * alone is not enough information.  For example, last check mail count may be
 * 3 and new mail count may be 3, but the user has read one mail message, and
 * a new mail message has been received since the last check.  By mail count
 * alone it appears that we have NOT received a new mail.  Returns true if
 * {\em newlist} contains at least one header that is not in {\em oldlist},
 * false otherwise.
 *
 * TODO: This would be more complete if we used the message UID in the case of
 * IMAP. But good enough.
 *
 * @param  newlist   C++ vector of {\em class T} elements
 * @param  oldlist   C++ vector of {\em class T} elements
 * @return           Boolean indicating whether there are elements in
 *                   {\em newlist} that are not in {\em oldlist}
 */
template<class T> gboolean 
Mailbox::contains_new (std::vector<T> newlist, std::vector<T> oldlist)
{
	// If newlist is larger then oldlist then we know newlist contains
	// elements not in oldlist. Fast check.
	if (newlist.size() > oldlist.size())
		return true;
       
	// If the two lists are exactly the same, then we know there are no
	// new elements in newlist.
	if (newlist == oldlist)
		return false;
       
	for (guint i=0; i< newlist.size(); i++) {
		gboolean foundmatch = false;
		for (guint j=0; j < oldlist.size(); j++) {
			if (newlist[i] == oldlist[j]) {
				foundmatch = true;
				break;
			}
		}

		// We did not find a match for newlist[i] in oldlist, so at least
		// newlist[i] is not in oldlist.
		if (!foundmatch)
			return true;
	}

	// newlist does not contain any headers that are not already in oldlist
	return false;
}

/**
 * Maximum number of lines to be read from the mail body. This value should be
 * greater than the value of the lines displayed;-) (see "src/ui-popup.cc").
 */
const gint bodyLinesToBeRead_=12;

/**
 * In some situations we need to read a certain number of lines from the
 * network to get the line we want. Unfortunately this number may vary in
 * reality because of the following reasons:
 *  * There is no limit for the response
 *  * There exist different extensions to the protocols
 *  * Not all servers implement protocols correctly
 *  * There is a DoS attack
 * To prevent being DoS attacked we need to set a limit of additional lines
 * that are being read. This is done by the following constant.
 */
const gint preventDoS_additionalLines_=16;

/**
 * To prevent being DoS attacked (see above): Maximum number of header lines
 * being read (POP3).
 */
const gint preventDoS_headerLines_=2048;

/**
 * To prevent being DoS attacked (see above): Limit for length of a read line.
 * SMTP: maximum line length is 1001 (see RFC 2821 4.5.3.1)
 * IMAP: no maximum line length
 * POP3: maximum response line length is 512 (see RFC 1939 3.)
 */
const gint preventDoS_lineLength_=16384;

#endif
