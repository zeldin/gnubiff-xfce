// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2010 Nicolas Rougier, 2004-2010 Robert Sowada
//
// This program is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef __SOCKET_H__
#define __SOCKET_H__

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#ifdef HAVE_LIBSSL
#  include <openssl/ssl.h>
#  include <openssl/err.h>
#endif

#include <glib.h>
#include <string>
#include "gnubiff_options.h"


const gint	SOCKET_TIMEOUT			=	 2;
const gint	SOCKET_STATUS_OK		=	 1;
const gint	SOCKET_STATUS_ERROR		=	 0;
const gint	SD_CLOSE				=	-1;

class Socket {

protected:
	std::string			hostname_;
	gushort				port_;
	class Mailbox *		mailbox_;
	guint				uin_;
	gboolean			use_ssl_;
	std::string			certificate_;
	/** Maximum length of a line being read from the socket before assuming
	 *  a DoS attack. When opening a connection this variable is set by
	 *  taking the value of the option "prevdos_line_length" (this is done
	 *  to avoid looking up the option for every line being read). */
	guint				prevdos_line_length_;
# ifdef HAVE_LIBSSL
	SSL_CTX *					context_;
	SSL *						ssl_;
	gboolean					bypass_certificate_;
	static class Certificate *	ui_cert_;
	static GStaticMutex			ui_cert_mutex_;		// Lock to avoid conflicts
# endif
	gint				sd_;
	gint				status_;

	gboolean connect (guint timeout);

public:
	/**
	 * base
	 **/
	Socket (class Mailbox *mailbox);
	virtual ~Socket (void);

	/**
	 * main   
	 **/
	gint open  (std::string hostname = "",
				gushort port = 0,
				guint authentication = AUTH_SSL,
				std::string certificate = "",
				guint timeout = 5);
	gint starttls (std::string certificate = "");
	void close (void);
	gint write (std::string line, gboolean print = true);
	gint read  (std::string &line,
				gboolean print = true,
				gboolean check = true);
  
	/**
	 * access
	 **/
	void status (const gint status)				{status_ = status;}
	gint status (void)							{return status_;}
	std::string hostname (void)					{return hostname_;}
	void set_read_timeout(gint timeout);

#ifdef HAVE_LIBSSL
	SSL *ssl (void)								{return ssl_;}
	const gboolean bypass_certificate (void)	{return bypass_certificate_;}
	void bypass_certificate (gboolean b)		{bypass_certificate_ = b;}
#endif
};

/* TEMP_FAILURE_RETRY seems to be available only on Linux. For systems that
 * don't have this macro we provide our own version. This code was taken from
 * file "/usr/include/unistd.h" from Debian package "libc6-dev"
 * version 2.3.2.ds1-20. */
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) \
	(__extension__ \
	 ({ long int __result; \
		do __result = (long int) (expression); \
		while (__result == -1L && errno == EINTR); \
		__result; }))
#endif


#endif
