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

#ifndef __IMAP4_H__
#define __IMAP4_H__

#include "mailbox.h"
#include "socket.h"

#define IMAP4(x)				((Imap4 *)(x))

/**
 * Mailbox for the IMAP4 protocol. 
 */
class Imap4 : public Mailbox {

 protected:
	/// Socket to talk to server
	class Socket *				socket_;
	/// Does the server support the IDLE capability?
	gboolean					idleable_;
	/// Is the server currently idled?
	gboolean					idled_;
	/// Tag created for the last sent IMAP command.
	std::string					tag_;
	/** Counter for creating the tag of the next IMAP command to be sent to
	 *  the server. */
	guint						tagcounter_;
 public:
	// ========================================================================
	//  base
	// ========================================================================

	Imap4 (class Biff *owner);
	Imap4 (const Mailbox &other);
	~Imap4 (void);

	// ========================================================================
	//  exceptions
	// ========================================================================

	/// General exception for IMAP mailbox. 
	class imap_err : public std::exception {};
	/** Exception for a socket connection failure. Usually this is thrown when
	 *  reading or writing. */
	class imap_socket_err : public imap_err {};
	/** Exception for a problem with a IMAP command. This exception may be
	 *  thrown in the following situations:
	 *  \begin{itemize}
	 *     \item There is an error when creating the line that is to be sent to
	 *           the server
	 *     \item There is an unexpected response by the server to the command
	 *     \item The command is not responded by OK
	 *  \end{itemize} */
	class imap_command_err : public imap_err {};
	/// This exception is thrown when a DoS attack is suspected. 
	class imap_dos_err : public imap_err {};
	/** This exception is thrown when login isn't possible. This can happen in
	 *  the following situations:
	 *  \begin{itemize}
	 *     \item The server doesn't want us to login (via the LOGINDISABLED
	 *           capability)
	 *     \item The user doesn't provide a password
	 *  \end{itemize} */
	class imap_nologin_err : public imap_err {};

	// ========================================================================
	//  main
	// ========================================================================

	virtual void threaded_start (guint delay = 0);
	void start (void);
	void fetch (void) throw (imap_err);
	void connect (void) throw (imap_err);
	void fetch_mails (void) throw (imap_err);
	
 private:
	// ========================================================================
	//	Internal stuff
	// ========================================================================	
	gboolean parse_bodystructure (std::string, class PartInfo &,
								  gboolean toplevel=true);
	gboolean parse_bodystructure_parameters (std::string, class PartInfo &);
	void command_capability (void) throw (imap_err);
	void command_fetchbody (guint, class PartInfo &,
							std::vector<std::string> &) throw (imap_err);
	PartInfo command_fetchbodystructure (guint) throw (imap_err);
	std::vector<std::string> command_fetchheader (guint) throw (imap_err);
	void command_login (void) throw (imap_err);
	std::vector<int> command_searchnotseen (void) throw (imap_err);
	void command_select (void) throw (imap_err);
	void command_waitforack (gint num=0) throw (imap_err);
	void reset_tag();
	std::string tag();
	gint send(std::string,gboolean debug=true);
	std::string idle_renew_loop() throw (imap_err);	 // Renew IDLE state
													 // periodically. 
	void update_applet();						 // Update the applet to new IMAP state.
	void idle() throw (imap_err);		         // Begin idle IMAP mode.
	void close();								 // Cleanup and close IMAP connection.
};

/**
 * Information about one part of a multi-part mail. If the mail consists only
 * of one part the information is valid for the whole mail.
 */
class PartInfo
{
 public:
	/** Part identifier as needed for the IMAP command FETCH (see
	 *  RFC 3501 6:4:5). This is the part of the mail that will be displayed
	 *  by gnubiff (if possible). */
	std::string part_;
	/// MIME type of this part. Currently only "text/plain" is supported.
	std::string mimetype_;
	/// Encoding of this part. Currently supported encodings are 7bit, 8bit, binary and quoted-printable.
	std::string encoding_;
	/// Character set of this part
	std::string charset_;
	/// Size of this part in bytes
	gint size_;
};

#endif
