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

#include <errno.h>
#include <sstream>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ui-certificate.h"
#include "mailbox.h"
#include "socket.h"
#include "nls.h"
#include "support.h"


// ========================================================================
//  Static features
// ========================================================================	
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

/**
 * Determine the IP addresses of the given host and port and establish
 * a connection to one of them.
 *
 * @param timeout timeout
 * @result        boolean indicating success
 */
gboolean 
Socket::connect (guint timeout)
{
	struct addrinfo		hints, *result, *rptr;
	int					i;
	std::stringstream	service;

	// Get address info
	service << port_;
	memset (&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	i = getaddrinfo (hostname_.c_str(), service.str().c_str(), &hints,
					 &result);
	if (i != 0) {
		g_warning (_("[%d] Unable to connect to %s on port %d"), uin_,
				   hostname_.c_str(), port_);
#ifdef debug
		g_message ("[%d] Call to getaddrinfo fails: %s", uin_,
				   gai_strerror(i));
#endif
		sd_ = SD_CLOSE;
		return false;
	}

	// Try all returned addresses
	for (rptr = result; rptr != NULL; rptr = rptr->ai_next) {
		// Try to create socket
		sd_ = socket (rptr->ai_family, rptr->ai_socktype, rptr->ai_protocol);
		if (sd_ == SD_CLOSE)
			continue;

		// Set non-blocking socket if a timeout is required
		if (timeout > 0) {
			int arg = fcntl (sd_, F_GETFL, NULL);
			arg |= O_NONBLOCK;
			fcntl (sd_, F_SETFL, arg);
		}

		// Try to connect
		i = ::connect (sd_, rptr->ai_addr, rptr->ai_addrlen);
		if (i != -1)
			break;

		// Connect failed but this may be because of non-blocking socket
		if (timeout > 0 && errno == EINPROGRESS) {
			int valopt;
			struct timeval tv;
			tv.tv_sec = timeout;
			tv.tv_usec = 0;
			fd_set myset;
			FD_ZERO (&myset);
			FD_SET (sd_, &myset);
			if (select (sd_ + 1, NULL, &myset, NULL, &tv) > 0) {
				socklen_t lon = sizeof (int);
				getsockopt (sd_, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon);
				if (!valopt) {
					// Set to blocking mode again...
					int arg = fcntl (sd_, F_GETFL, NULL);
					arg &= (~O_NONBLOCK);
					fcntl (sd_, F_SETFL, arg);
					break;
				}
			}
		}
		// Cannot connect, so close socket
		::close (sd_);
	}

	// Free temporary allocated memory
	freeaddrinfo (result);

	// No success with any address?
	if (!rptr) {
		g_warning (_("[%d] Unable to connect to %s on port %d"), uin_,
				   hostname_.c_str(), port_);
#ifdef debug
		g_message ("[%d] Cannot connect to any address", uin_);
#endif
		sd_ = SD_CLOSE;
		return false;
	}

	return true;
}

gint 
Socket::open (std::string hostname, gushort port, guint authentication,
			  std::string certificate, guint timeout)
{
	hostname_ = hostname;
	port_ = port;
	if ((authentication == AUTH_SSL) || (authentication == AUTH_CERTIFICATE))
		use_ssl_ = true;
	certificate_ = certificate;

	// Get options' values to avoid periodic lookups
	prevdos_line_length_=mailbox_->biff()->value_uint ("prevdos_line_length");

	// Default status before trying to connect
	status_ = SOCKET_STATUS_ERROR;

	// Connect to host on given port
#ifdef DEBUG
	g_message ("[%d] OPEN %s:%d", uin_, hostname_.c_str(), port_);
#endif
	if (!connect (timeout))
		return 0;

#ifdef HAVE_LIBSSL
	if (use_ssl_) {
		if (certificate_.size() > 0) {
			const gchar *capath = mailbox_->biff()->value_gchar ("dir_certificates");
			if (*capath == '\0')
				capath = NULL;
			if (!SSL_CTX_load_verify_locations (context_, certificate_.c_str(),
												capath)) {
				g_warning(_("[%d] Failed to load certificate (%s) for %s"),
						  uin_, certificate_.c_str(), hostname_.c_str());
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
			g_warning (_("[%d] Unable to connect to %s on port %d"), uin_,
					   hostname_.c_str(), port_);
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
Socket::starttls (std::string certificate)
{
#ifdef HAVE_LIBSSL
	char *err_buf;
	
	err_buf = (char*) malloc (120);
	
	context_ = SSL_CTX_new (TLSv1_client_method());

		if (certificate_.size() > 0) {
			const gchar *capath = mailbox_->biff()->value_gchar ("dir_certificates");
			if (*capath == '\0')
				capath = NULL;
			if (!SSL_CTX_load_verify_locations (context_, certificate_.c_str(),
												capath)) {
				g_warning(_("[%d] Failed to load certificate (%s) for %s"),
						  uin_, certificate_.c_str(), hostname_.c_str());
				::close (sd_);
				sd_ = SD_CLOSE;
				free(err_buf);
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
			g_warning (_("[%d] Unable to set file descriptor: %s"), uin_,
					   hostname_.c_str());
			free(err_buf);
			return 0;
		}

		if (SSL_connect (ssl_) != 1) {
			ERR_error_string(ERR_get_error(), err_buf);
			SSL_free (ssl_);
			ssl_ = NULL;
			::close (sd_);
			sd_ = SD_CLOSE;
			g_warning (_("[%d] Unable to negotiate TLS connection: %s"), uin_,
					   err_buf);
			return 0;
			free(err_buf);
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

	use_ssl_ = true;
	
#endif
	status_ = SOCKET_STATUS_OK;
	free(err_buf);
	return 1;
}

/**
 *  Close the socket.
 */
void 
Socket::close (void)
{
	// Is socket already closed?
	if (sd_ == SD_CLOSE) {
#ifdef HAVE_LIBSSL
		if (ssl_) {
			SSL_free (ssl_);
			ssl_ = NULL;
		}
#endif
		return;
	}

#ifdef DEBUG
	g_message ("[%d] CLOSE %s:%d", uin_, hostname_.c_str(), port_);
#endif

	// Read lines that are not yet read
	std::string line;
	fcntl (sd_, F_SETFL, O_NONBLOCK);
	guint cnt = 1 + mailbox_->biff()->value_uint ("prevdos_close_socket");
	do {
		read (line, false, false);
	} while ((!line.empty()) && (cnt--));

#ifdef HAVE_LIBSSL
	if (ssl_) {
		SSL_shutdown (ssl_);
		SSL_free (ssl_);
		ssl_ = NULL;
	}
#endif
	::close (sd_);
	sd_ = SD_CLOSE;
}

gint
Socket::write (std::string line, gboolean print)
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
	if (print)
		g_message ("[%d] SEND(%s:%d): %s", uin_, hostname_.c_str(), port_,
				   line.substr(0, line.size()-2).c_str());
#endif

	if ((print) && (status_ != SOCKET_STATUS_OK)) {
		g_warning (_("[%d] Unable to write to %s on port %d"), uin_, hostname_.c_str(), port_);
		close();
	}

	return status_;
}

gint 
Socket::read (std::string &line, gboolean print, gboolean check)
{
	// Do not allow writing to a closed, or invalid socket. This causes
	// SEGV in SSL.
	if (sd_ == SD_CLOSE)
		return SOCKET_STATUS_ERROR;

	char buffer;
	int status = 0;
	line = "";
	status_ = -1;

	gint cnt = 1 + prevdos_line_length_;

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
	if (print) {
		switch (status_) {
		case SOCKET_TIMEOUT:
			g_message ("[%d] RECV TIMEOUT(%s:%d)", uin_, hostname_.c_str(),
					   port_);
			break;
		case SOCKET_STATUS_OK:
			g_message ("[%d] RECV(%s:%d): %s", uin_, hostname_.c_str(), port_,
					   line.c_str());
			break;
		default:
			if (cnt <= 0)
				g_message ("[%d] RECV ERROR(%s:%d): line too long, "
					   "security/prevdos_line_length should be increased",
						   uin_, hostname_.c_str(), port_);
			else
				g_message ("[%d] RECV ERROR(%s:%d): %s", uin_,
						hostname_.c_str(), port_,
						strerror(status));
			break;
		}
	}
#endif
	if (status_ == SOCKET_STATUS_ERROR) {
		if (cnt <= 0)
			g_warning (_("[%d] line too long, security/prevdos_line_length "
				     "should be increased"), uin_);
		else
			g_warning (_("[%d] Unable to read from %s on port %d"),
					uin_, hostname_.c_str(), port_);
		close();
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
		g_warning (_("Could not set read timeout on socket: %s"),
				   strerror(errno));
}
