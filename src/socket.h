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

const int	SOCKET_STATUS_OK		=	 1;
const int	SOCKET_STATUS_ERROR		=	 0;
const int	SD_CLOSE				=	-1;


class Socket {

protected:
	std::string			hostname_;
	gushort				port_;
	class Mailbox *		mailbox_;
	guint				uin_;
	gboolean			use_ssl_;
	std::string			certificate_;
# ifdef HAVE_LIBSSL
	SSL_CTX *			context_;
	SSL *				ssl_;
	gboolean			bypass_certificate_;
	class Certificate *	ui_certificate_;
# endif
	gint				sd_;
	gint				status_;
	static GStaticMutex	hostname_mutex_;


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
				gboolean use_ssl = false,
				std::string certificate = "");
	gint close (void);
	gint write (std::string line,
				gboolean debug = true);
	gint read  (std::string &line,
				gboolean debug = true,
				gboolean check = true);

	/**
	 * access
	 **/
	void status (const gint status)				{status_ = status;}
	gint status (void)							{return status_;}
	std::string hostname (void)					{return hostname_;}
#ifdef HAVE_LIBSSL
	SSL *ssl (void)								{return ssl_;}
	const gboolean bypass_certificate (void)	{return bypass_certificate_;}
	void bypass_certificate (gboolean b)		{bypass_certificate_ = b;}
#endif
};

#endif
