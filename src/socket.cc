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

#include <sstream>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ui-certificate.h"
#include "mailbox.h"
#include "socket.h"
#include "nls.h"


GStaticMutex Socket::hostname_mutex_  = G_STATIC_MUTEX_INIT;

Socket::Socket (Mailbox *mailbox)
{
	mailbox_ = mailbox;
	if (mailbox_)
		uin_ = mailbox->uin();
	else
		uin_ = 0;
	hostname_ = "";
	port_ = 0;
	use_ssl_ = false;
	certificate_ = "";
	sd_ = SD_CLOSE;
	status_ = SOCKET_STATUS_ERROR;
#ifdef HAVE_LIBSSL
	ssl_ = 0;  
	SSL_library_init ();
	SSL_load_error_strings();
	context_ = SSL_CTX_new (SSLv23_client_method());
	bypass_certificate_ = false;
	ui_certificate_ = new Certificate ();
#endif
}

Socket::~Socket (void)
{
	if (sd_ != SD_CLOSE)
		close();
}	

gint
Socket::open (std::string hostname,
			  gushort port,
			  gboolean use_ssl,
			  std::string certificate)
{
	hostname_ = hostname;
	port_ = port;
	use_ssl_ = use_ssl;
	certificate_ = certificate;

	struct sockaddr_in sin;
	struct hostent *host;
	struct in_addr address;

#ifdef DEBUG
	g_message ("[%d] OPEN %s:%d", uin_, hostname_.c_str(), port_);
#endif

	// Default status before trying to connect
	status_ = SOCKET_STATUS_ERROR;

	// Create an endpoint for communication
	if ((sd_ = socket (AF_INET, SOCK_STREAM, IPPROTO_IP)) == -1) {
		sd_ = SD_CLOSE;
		g_warning (_("[%d] Unable to connect to %s on port %d"), uin_, hostname_.c_str(), port_);
		return 0;
	}

	// Setting socket info for connection
	memset ((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons (port_);

	// First, try to get the address by standard notation (e.g. 127.0.0.1)
	if (inet_aton (hostname_.c_str(), &address) == 0) {

		// If it does not work, get the address by name (e.g. localhost.localdomain)

		// Believe it or not but gethostbyname is not thread safe...
		g_static_mutex_lock (&hostname_mutex_);
		host = gethostbyname (hostname_.c_str());
		
		if (host == 0) {
			g_static_mutex_unlock (&hostname_mutex_);
			::close (sd_);
			sd_ = SD_CLOSE;
			g_warning (_("[%d] Unable to connect to %s on port %d"), uin_, hostname_.c_str(), port_);
			return 0;
		}

		// This way of filling sin_addr field avoid 'incompatible types in assignment' problems
		memcpy ((void *) &sin.sin_addr, *(host->h_addr_list), host->h_length);
	}
	else
		sin.sin_addr = address;

	// Initiate the connection on the socket
	if ((connect (sd_, (struct sockaddr *) &sin, sizeof (struct sockaddr_in))) == -1) {
		g_static_mutex_unlock (&hostname_mutex_);
		::close (sd_);
		sd_ = SD_CLOSE;
		g_warning (_("[%d] Unable to connect to %s on port %d"), uin_, hostname_.c_str(), port_);
		return 0;
	}

	g_static_mutex_unlock (&hostname_mutex_);

#ifdef HAVE_LIBSSL
	if (use_ssl_) {
		if (certificate_.size() > 0){
			if (!SSL_CTX_load_verify_locations (context_, certificate_.c_str(), NULL)) {
				g_warning(_("[%d] Failed to load certificate (%s) for %s"), uin_, certificate_.c_str(), hostname_.c_str());
				::close (sd_);
				sd_ = SD_CLOSE;
				return 0;
			}
			SSL_CTX_set_verify (context_, SSL_VERIFY_PEER, NULL);
		}
		else
			SSL_CTX_set_verify (context_, SSL_VERIFY_NONE, NULL);
    

		ssl_ = SSL_new (context_);
		if ((!ssl_) || (SSL_set_fd (ssl_, sd_) == 0)) {
			::close (sd_);
			sd_ = SD_CLOSE;
			g_warning (_("[%d] Unable to connect to %s on port %d"), uin_, hostname_.c_str(), port_);
			return 0;
		}
		if (SSL_connect (ssl_) != 1) {
			SSL_free (ssl_);
			ssl_ = NULL;
			::close (sd_);
			sd_ = SD_CLOSE;
			g_warning (_("[%d] Unable to connect to %s on port %d"), uin_, hostname_.c_str(), port_);
			return 0;
		}

		if ((certificate_.size() > 0) && (SSL_get_verify_result(ssl_) != X509_V_OK)) {
			ui_certificate_->select (this);
			if (!bypass_certificate_) {
				SSL_free (ssl_);
				ssl_ = NULL;
				::close (sd_);
				sd_ = SD_CLOSE;
				g_warning (_("[%d] Cannot identify remote host (%s on port %d)"), uin_, hostname_.c_str(), port_);
			}
		}		
		
	}
#endif
	status_ = SOCKET_STATUS_OK;
	return 1;
};


gint
Socket::close (void)
{
#ifdef DEBUG
	g_message ("[%d] CLOSE %s:%d", uin_, hostname_.c_str(), port_);
#endif
	std::string line;  
	if (sd_ != SD_CLOSE) {
		fcntl (sd_, F_SETFL, O_NONBLOCK);
		do {
			read (line, false, false);
		} while (!line.empty());
	}
  
#ifdef HAVE_LIBSSL
	if (use_ssl_ && ssl_) {
		if (sd_ != SD_CLOSE)
			SSL_shutdown (ssl_);
		SSL_free (ssl_);
		ssl_ = 0;
	}
#endif
	if(sd_ != SD_CLOSE)
		::close (sd_);
	sd_ = SD_CLOSE;
	return 1;
}


gint
Socket::write (std::string line,
			   gboolean debug)
{
	status_ = -1;

#ifdef HAVE_LIBSSL
	if (use_ssl_) {
		if (SSL_write (ssl_, line.c_str(), line.size()) <= 0)
			status_ = SOCKET_STATUS_ERROR;
		else
			status_ = SOCKET_STATUS_OK;
	}
#endif
	if (status_ == -1) {
		if (::write (sd_, line.c_str(), line.size()) <= 0)
			status_ = SOCKET_STATUS_ERROR;
		else
			status_ = SOCKET_STATUS_OK;
	}

#ifdef DEBUG
	if (debug)
		g_print ("** Message: [%d] SEND(%s:%d): %s", uin_, hostname_.c_str(), port_, line.c_str());
#endif

	if ((debug) && (!status_)) {
		g_warning (_("[%d] Unable to write to %s on port %d"), uin_, hostname_.c_str(), port_);
		close();
		mailbox_->status (MAILBOX_ERROR);
	}

	return status_;
}

gint
Socket::read (std::string &line,
			  gboolean debug,
			  gboolean check)
{
	char buffer;
	int status;
	line = "";
	status_ = -1;

#ifdef HAVE_LIBSSL
	if (use_ssl_) {
		while (((status = SSL_read (ssl_, &buffer, 1)) > 0) && (buffer != '\n'))
			line += buffer;
		if (status > 0)
			status_ = SOCKET_STATUS_OK;
		else
			status_ = SOCKET_STATUS_ERROR;
	}
#endif
	if (status_ == -1) {
		while (((status = ::read (sd_, &buffer, 1)) > 0) && (buffer != '\n'))
			line += buffer;
		if (status > 0)
			status_ = SOCKET_STATUS_OK;
		else
			status_ = SOCKET_STATUS_ERROR;
	}

	if (!check)
		return status_;

#ifdef DEBUG
	if (debug)
		g_message ("[%d] RECV(%s:%d): %s", uin_, hostname_.c_str(), port_, line.c_str());
#endif
	if ((debug) & (!status_)) {
		g_warning (_("[%d] Unable to read from %s on port %d"), uin_, hostname_.c_str(), port_);
		close();
		mailbox_->status (MAILBOX_ERROR);
	}

	// Check imap4
	if (mailbox_->protocol() == PROTOCOL_IMAP4) {
		if (line.find ("* BYE") == 0) {
			close();
			status_ = SOCKET_STATUS_ERROR;
			mailbox_->status (MAILBOX_ERROR);
		}
	}

	// Check pop
	if ((mailbox_->protocol() == PROTOCOL_APOP) || (mailbox_->protocol() == PROTOCOL_POP3)) {
		 if (line.find ("-ERR") == 0) {
			 close();
			 status_ = SOCKET_STATUS_ERROR;
			 mailbox_->status (MAILBOX_ERROR);
		 }
	}
	
	return status_;
}
