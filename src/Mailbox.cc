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
#include <sstream>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Mailbox.h"


// ================================================================================
//  Callbacks (C binding)
// ================================================================================
extern "C" {
	gboolean MAILBOX_watch_timeout (gpointer data)	{((Mailbox *) data)->watch_timeout (); return false;}
	gpointer MAILBOX_watch (gpointer data)			{((Mailbox *) data)->watch_thread (); return 0;}
}


// ================================================================================
//  Constructors & Destructors
// --------------------------------------------------------------------------------
//  
// ================================================================================
Mailbox::Mailbox (Biff *owner)
{
	_owner = owner;
	_name = _("Account ");
	std::ostringstream s;
	s << owner->mailboxes()+1;
	_name +=  s.str();
	_protocol = PROTOCOL_FILE; // will be overriden by specific constructors
	_port = 0;
#ifdef HAVELIBSSL
	_use_ssl = true;
#else
	_use_ssl = false;
#endif
	_address = "";
	if (g_getenv("MAIL"))
		_address = g_getenv ("MAIL");
	_user = "";
	if (g_get_user_name())
		_user = g_get_user_name();
	_password = "";
	_certificate = "";
	_polltime = 60;

	_unread.clear();
	_status = MAILBOX_EMPTY;
	_sd = SD_CLOSE;
	_hidden.clear();
	_seen.clear();
	_polltag = 0;
	_watch_mutex = g_mutex_new();
	_object_mutex = g_mutex_new();
#ifdef HAVE_LIBSSL
	_ssl = 0;  
	SSL_library_init ();
	SSL_load_error_strings();
	_context = SSL_CTX_new (SSLv23_client_method());
#endif
}

Mailbox::Mailbox (Mailbox *other)
{
	_owner = other->_owner;
	_name =  other->_name;
	_port = other->_port;
	_use_ssl = other->_use_ssl;
	_address = other->_address;
	_user = other->_user;
	_password = other->_password;
	_certificate = other->_certificate;
	_polltime = other->_polltime;
	_unread.clear();
	_status = MAILBOX_EMPTY;
	_sd = SD_CLOSE;
	_hidden.clear();
	_seen.clear();
	_polltag = 0;
	_watch_mutex = g_mutex_new();
	_object_mutex = g_mutex_new();
#ifdef HAVE_LIBSSL
	_ssl = 0;  
	SSL_library_init ();
	SSL_load_error_strings();
	_context = SSL_CTX_new (SSLv23_client_method());
#endif
}

Mailbox::~Mailbox (void)
{
	watch_off();
	// We do not delete this object until we can get a grip on the watch mutex
	g_mutex_lock (_watch_mutex);
	g_mutex_unlock (_watch_mutex);
	g_mutex_free (_watch_mutex);
	g_mutex_lock (_object_mutex);
	g_mutex_unlock (_object_mutex);
	g_mutex_free (_object_mutex);
}

GStaticMutex Mailbox::_hostname_mutex  = G_STATIC_MUTEX_INIT;



// ================================================================================
//  Watch functions
// --------------------------------------------------------------------------------
//  Those function handles automatic and forced watch
// ================================================================================
void Mailbox::watch (void)
{
	GError *err = NULL;
#ifdef DEBUG
	g_message ("Attempting to create a thread...");
#endif
	g_thread_create (MAILBOX_watch, this, FALSE, &err);
	if (err != NULL)  {
		g_error (_("Unable to create thread: %s\n"), err->message);
		g_error_free (err);
	} 
#ifdef DEBUG
	else
		g_message ("Thread creation went ok");
#endif
}

void Mailbox::watch_on (void)
{
	g_mutex_lock (_object_mutex);
	if (_polltag == 0)
		_polltag = g_timeout_add (_polltime*1000, MAILBOX_watch_timeout, this);
	g_mutex_unlock (_object_mutex);
}

void Mailbox::watch_off (void) {
	g_mutex_lock (_object_mutex);
	if (_polltag > 0)
		g_source_remove (_polltag);
	_polltag = 0;
	g_mutex_unlock (_object_mutex);
}

void Mailbox::mark_all (void) {
	g_mutex_lock (_object_mutex);
	_hidden.clear();
	_hidden = _seen;
	_unread.clear();
	_owner->save();
	g_mutex_unlock (_object_mutex);
}

gboolean Mailbox::watch_timeout (void) {
	g_thread_create (MAILBOX_watch, this, FALSE, 0);
	return false;
}


// ================================================================================
//  Watch thread function
// --------------------------------------------------------------------------------
//  This function is used within a thread so we need to take care that no other
//  thread has been started before. This is the purpose of the watch mutex
// ================================================================================
void Mailbox::watch_thread (void) {
	if (!g_mutex_trylock (_watch_mutex))
		return;
  
	// Stop automatic watch
	watch_off();

	// Nobody should access status in write mode but this thread, so we're safe
	get_status();

#ifdef DEBUG
	if (_status == MAILBOX_ERROR)	
		g_print ("Mailbox error\n");
	else if (_status == MAILBOX_OLD)
		g_print ("Mailbox old\n");
	else if (_status == MAILBOX_EMPTY)
		g_print ("Mailbox empty\n");
	else
		g_print ("Mailbox new\n");
#endif

	// Do we need to get headers ?
	if (_status == MAILBOX_EMPTY) {
		_unread.clear();
		_seen.clear();
	}
	else if ((_status != MAILBOX_ERROR) && (_status != MAILBOX_OLD))
		get_header();

	gdk_threads_enter();
	_owner->applet()->process();
	_owner->applet()->update();
	gdk_threads_leave();
	g_mutex_unlock (_watch_mutex);
}


// ================================================================================
//  Parse a mail string array
// --------------------------------------------------------------------------------
//  This function parse a mail (as a vector of string) to extract
//  sender/date/subject, to convert strings if necessary and to store the mail in
//  unread array (depending if it's spam or if it is internally marked as seen)
// ================================================================================
void Mailbox::parse (std::vector<std::string> &mail, int status)
{
	header h;
	h.status = status;

	for (guint i=0; i<mail.size(); i++) {
		gchar *buffer = g_ascii_strdown (mail[i].c_str(), -1);
		std::string line = buffer;
		g_free (buffer);
		
		// Sender
		// There should a whitespace or a tab after "From:", so we look
		// for "From:" and get string beginning at 6
		if ((mail[i].find ("From:") == 0) && h.sender.empty()) {
			if (mail[i].size() > 6)
				h.sender = mail[i].substr (6);
			else
				h.sender = _("<no sender found>");
		}

		// Subject
		// There should a whitespace or a tab after "Subject:", so we look
		// for "Subject:" and get string beginning at 9
		else if ((mail[i].find ("Subject:") == 0) && h.subject.empty()) {
			if (mail[i].size() > 9)
				h.subject = mail[i].substr (9);
			else
				h.subject = _("<no subject found>");
		}

		// Date
		// There should a whitespace or a tab after "Date:", so we look
		// for "Date:" and get string beginning at 6
		else if ((mail[i].find ("Date:") == 0) && h.date.empty()) {
			if (mail[i].size() > 6)
				h.date = mail[i].substr (6);
			else
				h.date = _("<no date found>");
		}

		// Charset
		// FIXME: Currently, this code does not handle the case where a
		// charset is coded over two or more lines. Question: is is a big
		// problem ? (is it allowed anyway ?)
		else if ((line.find ("charset=") != std::string::npos) && h.charset.empty()) {
			std::string charset = line.substr (int(line.find ("charset="))+8);
			// First we remove any leading '"'
			if (charset[0] == '\"')
				charset = charset.substr(1);

			// Then we wait for the end of charset
			for (guint j=0; j<charset.size(); j++) {
				if ((charset[j+1] == ';')  || (charset[j+1] == '\"') ||
					(charset[j+1] == '\n') || (charset[j+1] == '\t') ||
					(charset[j+1] == ' ')  || (charset[j+1] == '\0')) {
					h.charset = charset.substr (0, j+1);
					break;
				}
			}
		}
		// Status
		else if ((mail[i].find ("Status: R") == 0) && h.status == -1)
			h.status = MAIL_READ;
		else if (mail[i].find ("X-Mozilla-Status: 0001") == 0)
			h.status = MAIL_READ;
		else if (line.find ("x-spam-flag: yes") != std::string::npos)
			h.status = MAIL_READ;
		else if ((mail[i].empty()) && h.body.empty()) {
			guint j = 0;
			do {
				h.body += mail[i++] + std::string("\n");
				j++;
			} while ((j<10) && (i < mail.size()));
			if (j == 10)
				h.body += std::string("...");
		}
	}

	// Store mail depending on status
	if ((h.status == MAIL_UNREAD) || (h.status == -1))
		// Pines trick
		if (h.subject.find("DON'T DELETE THIS MESSAGE -- FOLDER INTERNAL DATA") == std::string::npos) {
			// Ok, at this point mail is
			//  - not a spam
			//  - has not been read (R or 0001 status flag)
			//  - is not a special header (see above)
			// so we have to decide what to do with it because we may have already displayed it
			// and maybe it has been gnubifficaly marked as "seen".
			//
			guint mailid = g_str_hash (h.sender.c_str()) ^ g_str_hash (h.subject.c_str()) ^ g_str_hash (h.date.c_str());
			guint j;
			for (j=0; j<_hidden.size(); j++)
				if (_hidden[j] == mailid)
					break;
			if (j >= _hidden.size())
				_unread.push_back(h);
			_seen.push_back (mailid);
		}
}


// ================================================================================
//  Some convenience function for socket handling
// --------------------------------------------------------------------------------
//  Those function are not really related to all mailboxes but if they were not
//  thre, another class (like NetMailbox shoudl have been created).
// ================================================================================
gint Mailbox::socket_open (std::string &hostname, unsigned short int port)
{
	struct sockaddr_in sin;
	struct hostent *host;
	struct in_addr address;

	// Default status before trying to connect
	_socket_status = SOCKET_STATUS_ERROR;

	// Create an endpoint for communication
	if ((_sd = socket (AF_INET, SOCK_STREAM, IPPROTO_IP)) == -1) {
		_sd = SD_CLOSE;
		g_warning (_("Unable to connect to %s on port %d"), hostname.c_str(), port);
		return 0;
	}

	// Setting socket info for connection
	memset ((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons (port);

	// First, try to get the address by standard notation (e.g. 127.0.0.1)
	if (inet_aton (hostname.c_str(), &address) == 0) {

		// If it does not work, get the address by name (e.g. localhost.localdomain)

		// Believe it or not but gethostbyname is not thread safe...
		g_static_mutex_lock (&_hostname_mutex);
		host = gethostbyname (hostname.c_str());
		
		if (host == 0) {
			g_static_mutex_unlock (&_hostname_mutex);
			close (_sd);
			_sd = SD_CLOSE;
			g_warning (_("Unable to connect to %s on port %d"), hostname.c_str(), port);
			return 0;
		}

		// This way of filling sin_addr field avoid 'incompatible types in assignment' problems
		memcpy ((void *) &sin.sin_addr, *(host->h_addr_list), host->h_length);
	}
	else
		sin.sin_addr = address;

	// Initiate the connection on the socket
	if ((connect (_sd, (struct sockaddr *) &sin, sizeof (struct sockaddr_in))) == -1) {
		g_static_mutex_unlock (&_hostname_mutex);
		close (_sd);
		_sd = SD_CLOSE;
		g_warning (_("Unable to connect to %s on port %d"), hostname.c_str(), port);
		return 0;
	}

	g_static_mutex_unlock (&_hostname_mutex);

#ifdef HAVE_LIBSSL
	if (_use_ssl) {
		if (_certificate.size() > 0){
			if (SSL_CTX_load_verify_locations(_context, _certificate.c_str(),	NULL) == 0) {
				g_warning("Failed to load server (%s) certificate (%s)", hostname.c_str(), _certificate.c_str());
				close (_sd);
				_sd = SD_CLOSE;
				return 0;
			}
			SSL_CTX_set_verify (_context, SSL_VERIFY_PEER, NULL);
		}
		else {
			SSL_CTX_set_verify (_context, SSL_VERIFY_NONE, NULL);
		}
    
		_ssl = SSL_new (_context);
		if (!_ssl){
			close (_sd);
			_sd = SD_CLOSE;
			g_warning (_("Unable to connect to %s on port %d"), hostname.c_str(), port);
			return 0;
		}
		if (SSL_set_fd (_ssl, _sd) == 0) {
			close (_sd);
			_sd = SD_CLOSE;
			g_warning (_("Unable to connect to %s on port %d"), hostname.c_str(), port);
			return 0;
		}
		if (SSL_connect (_ssl) != 1){
			g_warning("Failed to connect to server");
			SSL_free (_ssl);
			_ssl = NULL;
			close (_sd);
			_sd = SD_CLOSE;
			g_warning (_("Unable to connect to %s on port %d"), hostname.c_str(), port);
			return 0;
		}
	}
#endif
	_socket_status = SOCKET_STATUS_OK;
	return 1;
};

// ================================================================================
gint Mailbox::socket_close (void)
{
	std::string line;  
	if(_sd != SD_CLOSE) {
		fcntl (_sd, F_SETFL, O_NONBLOCK);
		do {
			socket_read (line, true);
		} while (!line.empty());
	}
  
#ifdef HAVE_LIBSSL
	if (_use_ssl && _ssl) {
		if (_sd != SD_CLOSE)
			SSL_shutdown (_ssl);
		SSL_free (_ssl);
		_ssl = 0;
	}
#endif
	if(_sd != SD_CLOSE)
		close (_sd);
	_sd = SD_CLOSE;
	return 1;
}

// ================================================================================
gint Mailbox::socket_write (std::string line, gboolean nodebug)
{
	_socket_status = SOCKET_STATUS_ERROR;

#ifdef DEBUG
	if (!nodebug)
		g_message ("SEND(%s:%d): %s", _address.c_str(), _port, line.c_str());
#endif

#ifdef HAVE_LIBSSL
	if (_use_ssl) {
		if (SSL_write (_ssl, line.c_str(), line.size()) <= 0) {
			g_warning (_("Unable to write to %s on port %d"), _address.c_str(), _port);
			return 0;
		}
		_socket_status = SOCKET_STATUS_OK;
		return 1;
	}
#endif
	if (write (_sd, line.c_str(), line.size()) <= 0) {
		g_warning (_("Unable to write to %s on port %d"), _address.c_str(), _port);
		return 0;
	}
	_socket_status = SOCKET_STATUS_OK;
	return 1;
}

// ================================================================================
//  The no_debug flag is used when closing the socket. At this precise time, we
//  we tried to "eat up" any remaining message until it's empty. Without this flag,
//  we would get some warning.
//
// ================================================================================
gint Mailbox::socket_read (std::string &line, gboolean nodebug)
{
	char buffer;
	int status;
	line = "";
	_socket_status = SOCKET_STATUS_ERROR;
#ifdef HAVE_LIBSSL
	if (_use_ssl) {
		while (((status = SSL_read (_ssl, &buffer, 1)) > 0) && (buffer != '\n'))
			line += buffer;
		if (status > 0) 
			_socket_status = SOCKET_STATUS_OK;
		else if (!nodebug) {
			g_warning (_("Unable to read from %s on port %d"), _address.c_str(), _port);
			return -1;
		}
		return status;
	}
#endif
	while (((status = read (_sd, &buffer, 1)) > 0) && (buffer != '\n'))
		line += buffer;
	if (status >= 0) 
		_socket_status = SOCKET_STATUS_OK;
	else if (!nodebug) {
		g_warning (_("Unable to read from %s on port %d"), _address.c_str(), _port);
		return -1;
	}
	return status;
}
