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
#ifndef _MAILBOX_H
#define _MAILBOX_H

#ifdef HAVE_CONFIG_H
#   include "../config.h"
#endif
#include <vector>
#ifdef HAVE_LIBSSL
#  include <openssl/ssl.h>
#  include <openssl/err.h>
#endif
#include <glib.h>
#include <sstream>
#include <string>
#include "Biff.h"


// ===================================================================
// - Constant definitions --------------------------------------------
// ===================================================================
const int	MAILBOX_ERROR			=	0;
const int	MAILBOX_EMPTY			=	1;
const int	MAILBOX_OLD				=	2;
const int	MAILBOX_NEW				=	3;
const int	MAILBOX_CHECKING		=	4;

const int	MAIL_UNREAD				=	0;
const int	MAIL_READ				=	1;

const int	SOCKET_STATUS_OK		=	0;
const int	SOCKET_STATUS_ERROR		=	1;

const int	SD_CLOSE				=	-1;


class Mailbox {

	// ===================================================================
	// - Public methods --------------------------------------------------
	// ===================================================================
 public:
	Mailbox (class Biff *owner);	// New mailbox
	Mailbox (Mailbox *other);		// New mailbox (copy of other but the protocol)
	virtual ~Mailbox (void);		// Destroy mailbox
	void watch (void);				// Watch for new mail (now !)
	void watch_on (void);			// Start automatic watch
	void watch_off(void);			// Stop automatic watch
	void mark_all (void);			// Mark all stored mail as seen

	gboolean watch_timeout (void);
	void watch_thread (void);

	std::string &			name (void)			{return _name;}
	guint &					protocol (void)		{return _protocol;}
	guint &					port (void)			{return _port;}
	gboolean &				use_ssl	 (void)		{return _use_ssl;}
	std::string &			address	 (void)		{return _address;}
	std::string	&			folder (void)		{return _folder;}
	std::string	&			user (void)			{return _user;}
	std::string	&			password (void)		{return _password;}
	std::string	&			certificate (void)	{return _certificate;}
	guint &					polltime (void)		{return _polltime;}
	gint &					status (void)		{return _status;}

	std::vector<header> &	unread (void)		{return _unread;}
	header &				unread (int i)		{return _unread[i];}
	guint					unreads (void)		{
		g_mutex_lock (_object_mutex);
		guint s = _unread.size();
		g_mutex_unlock (_object_mutex);
		return s;
	}

	std::vector<guint> &	hidden (void)		{return _hidden;}
	guint &					hidden (int i)		{return _hidden[i];}
	guint					hiddens (void)		{return _hidden.size();}


	// ===================================================================
	// - Protected methods -----------------------------------------------
	// ===================================================================
 protected:
	virtual void	get_status		(void) = 0;
	virtual void	get_header		(void) = 0;
	void			parse	(std::vector<std::string> &mail,
							 int status = -1);

	gint socket_open  (std::string &hostname, unsigned short int port);
	gint socket_close (void);
	gint socket_write (std::string line, gboolean nodebug=false);
	gint socket_read  (std::string &line, gboolean nodebug=false);


	// ===================================================================
	// - Protected attributes --------------------------------------------
	// ===================================================================
 protected:
	// Description of mailbox
	std::string					_name;			// Name of the mailbox
	guint						_protocol;		// Protocol to be used
	guint						_port;			// Port to be used with apop, pop3 or imap protocols
	gboolean					_use_ssl;		// Use of SSL or not for apop, pop3 or imap4 protocols
	std::string					_address;		// Address of the mailbox (file or hostname)
	std::string					_folder;		// Sub-folder to look at for Imap4 and Maildir protocols
	std::string					_user;			// User name for apop, pop3 or imap4 protocols
	std::string					_password;		// Password for apop, pop3 or imap4 protocols
	std::string					_certificate;	// Filename of the certificate (empty for none)
	guint						_polltime;		// Time (seconds) between polls

	// Internal stuff
	class Biff *				_owner;			// Owner of this interface
	gint						_polltag;		// Tag for poll timer
	GMutex *					_object_mutex;	// Mutex for object features access
	GMutex *					_watch_mutex;	// Mutex for watch function control
	gint 						_status;		// Status of the mailbox
	std::vector<header>			_unread;		// Collected unread mail
	std::vector<guint>			_hidden;		// Mails that won't be displayed
	std::vector<guint>			_seen;			// Mails already seen           
#ifdef HAVE_LIBSSL
	SSL_CTX *					_context;		// OpenSSL context object
	SSL *						_ssl;			// OpenSSL socket object
#endif
	int							_sd;			// Regular socket descriptor
	int							_socket_status;	// Socket status

	static GStaticMutex			_hostname_mutex;// because gethostbyname is not thread safe with glibc
};

#endif
