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

#include "support.h"

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


// ========================================================================
//  Static features
// ========================================================================	
GStaticMutex Socket::hostname_mutex_  = G_STATIC_MUTEX_INIT;
#ifdef HAVE_LIBSSL
Certificate *Socket::ui_cert_ = 0;
GStaticMutex Socket::ui_cert_mutex_ = G_STATIC_MUTEX_INIT;
#endif

// ========================================================================
//  base
// ========================================================================	
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
	g_static_mutex_lock (&ui_cert_mutex_);
	if (ui_cert_ == 0)
		ui_cert_ = new Certificate ();
	g_static_mutex_unlock (&ui_cert_mutex_);
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
			  gint authentication,
			  std::string certificate,
			  guint timeout)
{
	hostname_ = hostname;
	port_ = port;
	if ((authentication == AUTH_SSL) || (authentication == AUTH_CERTIFICATE))
		use_ssl_ = true;
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

	// Set non-blocking socket if timeout required
	if (timeout > 0) {
		int arg = fcntl (sd_, F_GETFL, NULL);
		arg |= O_NONBLOCK;
		fcntl (sd_, F_SETFL, arg);
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


	int res = connect (sd_, (struct sockaddr *) &sin, sizeof (struct sockaddr_in));
	if ((timeout) && (res < 0)) {
		if (errno == EINPROGRESS) {
			int valopt;
			struct timeval tv;
			tv.tv_sec = timeout;
			tv.tv_usec = 0;
			fd_set myset;
			FD_ZERO (&myset);
			FD_SET (sd_, &myset);
			if (select (sd_+1, NULL, &myset, NULL, &tv) > 0) {
				socklen_t lon = sizeof (int);
				getsockopt (sd_, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon);
				if (valopt) {
					g_static_mutex_unlock (&hostname_mutex_);
					::close (sd_);
					sd_ = SD_CLOSE;
					g_warning (_("[%d] Unable to connect to %s on port %d"), uin_, hostname_.c_str(), port_);
					return 0;
				}
			}
			else {
				g_static_mutex_unlock (&hostname_mutex_);
				::close (sd_);
				sd_ = SD_CLOSE;
				g_warning (_("[%d] Unable to connect to %s on port %d"), uin_, hostname_.c_str(), port_);
				return 0;
			}
		}
		else {
			g_static_mutex_unlock (&hostname_mutex_);
			::close (sd_);
			sd_ = SD_CLOSE;
			g_warning (_("[%d] Unable to connect to %s on port %d"), uin_, hostname_.c_str(), port_);
			return 0;
		}
		// Set to blocking mode again...
		int arg = fcntl (sd_, F_GETFL, NULL);
		arg &= (~O_NONBLOCK);
		fcntl (sd_, F_SETFL, arg);
	}
	else if (res == -1) {
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
				g_warning(_("[%d] Failed to load certificate (%s) for %s"), uin_, hostname_.c_str(), certificate_.c_str(), hostname_.c_str());
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
			g_static_mutex_lock (&ui_cert_mutex_);
			ui_cert_->select (this);
			g_static_mutex_unlock (&ui_cert_mutex_);
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
}


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
	// Do not allow writing to a closed, or invalid socket. This causes
	// SEGV in SSL.
	if (sd_ == SD_CLOSE)
		return SOCKET_STATUS_ERROR;

	status_ = -1;

	// TEMP_FAILURE_RETRY will re-call the method if the write primitive
	// is interrupted by a signal.
	
#ifdef HAVE_LIBSSL
	if (use_ssl_) {
		if (TEMP_FAILURE_RETRY(SSL_write (ssl_, line.c_str(), line.size())) <= 0)
			status_ = SOCKET_STATUS_ERROR;
		else
			status_ = SOCKET_STATUS_OK;
	}
#endif
	if (status_ == -1) {
		if (TEMP_FAILURE_RETRY(::write (sd_, line.c_str(), line.size())) <= 0)
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
	// Do not allow writing to a closed, or invalid socket. This causes
	// SEGV in SSL.
	if (sd_ == SD_CLOSE)
		return SOCKET_STATUS_ERROR;

	char buffer;
	int status = 0;
	line = "";
	status_ = -1;

	gint cnt=1+preventDoS_lineLength_; 

	// TEMP_FAILURE_RETRY will re-call the method if the read primitive
	// is interrupted by a signal.
	
	errno = 0;
#ifdef HAVE_LIBSSL
	if (use_ssl_) {
		while ((0<cnt--)
			   && ((status=TEMP_FAILURE_RETRY(SSL_read (ssl_, &buffer, 1)))>0)
			   && (buffer != '\n'))
			line += buffer;
	}
	else
#endif
	{
		while ((0<cnt--)
			   && ((status=TEMP_FAILURE_RETRY(::read (sd_, &buffer, 1))) > 0)
			   && (buffer != '\n'))
			line += buffer;
	}

	if (errno == EAGAIN)
		status_ = SOCKET_TIMEOUT;
	else if ((status > 0) && (cnt>=0))
		status_ = SOCKET_STATUS_OK;
	else
		status_ = SOCKET_STATUS_ERROR;
	
	if (!check)
		return status_;

#ifdef DEBUG
	if (debug)
		if (status_ == SOCKET_TIMEOUT)
			g_message ("[%d] RECV TIMEOUT(%s:%d)", uin_, hostname_.c_str(), port_);
		else if (status_ == SOCKET_STATUS_OK)
			g_message ("[%d] RECV(%s:%d): %s", uin_, hostname_.c_str(), port_, line.c_str());
		else
			g_message ("[%d] RECV ERROR(%s:%d): %s", uin_, hostname_.c_str(), port_, strerror(status));
#endif
	if (status_ == SOCKET_STATUS_ERROR) {
		g_warning (_("[%d] Unable to read from %s on port %d"), uin_, hostname_.c_str(), port_);
		close();
		mailbox_->status (MAILBOX_ERROR);
	}

	// NOTE: It would be my take that there should not be mailbox specific
	// handling in the socket processing.	 -Byron
	
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

/**
 * Specify a timeout value for read operations on this socket.  Read
 * operations will block until the given time has expired.  If the
 * {\em gint Socket::read (std::string, gboolean, gboolean)} method
 * returns because of the timeout it will return with {\em SOCKET_TIMEOUT}.
 *
 * @param  timeout   Time in seconds for timeout duration.
 */
void 
Socket::set_read_timeout(gint timeout)
{
	struct timeval tv;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	if (setsockopt(sd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1)
		g_error("Could not set read timeout on socket: %s", strerror(errno));
}
