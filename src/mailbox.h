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
const gint	MAILBOX_CHECKING		=	4;
const gint	MAILBOX_BLOCKED			=	5;
const gint	MAILBOX_UNKNOWN			=	6;

const gint	MAIL_UNREAD				=	0;
const gint	MAIL_READ				=	1;

#define MAILBOX(x)					((Mailbox *)(x))


class Mailbox {

protected:
	std::string					name_;			// displayed name
	guint						protocol_;		// protocol

	gboolean					is_local_;		// is mailbox local ?
	std::string					location_;		// location
	std::string					hostname_;		// hostname
	guint						port_;			// port to connect to
	std::string					folder_;		// folder to check

	std::string					username_;		// username
	std::string					password_;		// password
	gboolean					use_ssl_;		// use SSL or not
	std::string					certificate_;	// certificate file
	guint						polltime_;		// delay between polls

	gboolean					listed_;		// listed or not in ui
	guint						uin_;			// unique identifier number
	static guint				uin_count_;		// unique identifier number count

	// Internal stuff
	class Biff *				biff_;			// biff owner
	gint						polltag_;		// tag for poll timer
	GMutex *					object_mutex_;	// mutex for object features access
	GMutex *					watch_mutex_;	// mutex for watch function control
	gint 						status_;		// status of the mailbox
	std::vector<header>			unread_;		// collected unread mail
	std::vector<header>			new_unread_;	// collected unread mail (tmp buffer)
	std::vector<guint>			hidden_;		// mails that won't be displayed
	std::vector<guint>			seen_;			// mails already seen   
	std::vector<guint>			new_seen_;		// mails already seen (tmp buffer)

	static class Authentication *ui_authentication_;// ui to get username & password

public:
	/* base */
	Mailbox (class Biff *biff);
	Mailbox (const Mailbox &other);
	virtual ~Mailbox (void);

	/* main */
	virtual void get_status (void);					// get status of mailbox
	virtual void get_header (void);					// get mail from mailbox
	void watch (void);								// immediately start watch thread
	void watch_on (guint delay = 0);				// start automatic watch
	void watch_off(void);							// stop automatic watch
	void mark_all (void);							// internally mark all mail as seen
	gboolean watch_timeout (void);					// timeout
	void watch_thread (void);						// watch thread method
	void parse (std::vector<std::string> &mail,		// parse a mail to extract fields
				int status = -1);					//  and 10 first lines

	/* format detection */
	void lookup (void);								// immediately start lookup thread
	void lookup_thread (void);						// lookup for mailbox format


	/* access*/
	const std::string name (void)						{return name_;}
	void name (const std::string name)					{name_ = name;}

	const gboolean is_local (void)						{return is_local_;}
	void is_local (const gboolean is_local)				{is_local_ = is_local;}
	const std::string location (void)					{return location_;}
	void location (const std::string location)			{location_ = location;}
	const std::string hostname (void)					{return hostname_;}
	void hostname (const std::string hostname)			{hostname_ = hostname;}
	const guint port (void)								{return port_;}
	void port (const guint port)						{port_ = port;}
	const std::string folder (void)						{return folder_;}
	void folder (const std::string folder)				{folder_ = folder;}

	const std::string username (void)					{return username_;}
	void username (const std::string username)			{username_ = username;}
	const std::string password (void)					{return password_;}
	void password (const std::string password)			{password_ = password;}
	const std::string certificate (void)				{return certificate_;}
	const gboolean use_ssl (void)						{return use_ssl_;}
	void use_ssl (const gboolean use_ssl)				{use_ssl_ = use_ssl;}
	void certificate (const std::string certificate)	{certificate_ = certificate;}
	const guint polltime (void)							{return polltime_;}
	void polltime (const guint polltime)				{polltime_ = polltime;}
	
	const gint status (void) 							{return status_;}
	void status (const gint status)						{status_ = status;}
	const guint protocol (void)							{return protocol_;}
	void protocol (const guint protocol)				{protocol_ = protocol;}	
	const gboolean listed (void)						{return listed_;}
	void listed (gboolean listed)						{listed_ = listed;}
	const guint uin (void)								{return uin_;}

	std::vector<header> &unread (void)					{return unread_;}
	header &unread (int i)								{return unread_[i];}
	guint unreads (void)
	{
		g_mutex_lock (object_mutex_);
		guint s = unread_.size();
		g_mutex_unlock (object_mutex_);
		return s;
	}
	std::vector<guint> &hidden (void)					{return hidden_;}
	guint &hidden (int i)								{return hidden_[i];}
	guint hiddens (void)								{return hidden_.size();}

};

#endif
