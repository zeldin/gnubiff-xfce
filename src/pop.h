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

#ifndef __POP_H__
#define __POP_H__

#include "mailbox.h"
#include <set>

#define POP(x)					((Pop *)(x))

/**
 * Mailbox for the POP3 and APOP protocols. This serves only as a base for
 * the more specific mailboxes for these protocols.
 */
class Pop : public Mailbox {
protected:
	/// Socket to talk to the server
	class Socket *	 			socket_;

public:
	// ========================================================================
	//  base
	// ========================================================================	
	Pop (class Biff *biff);
	Pop (const Mailbox &other);
	virtual ~Pop (void);

	// ========================================================================
	//  exceptions
	// ========================================================================
	/** Generic exception for POP3 and APOP mailboxes serving as a base for
	 *  more specific exceptions. */
	class pop_err : public mailbox_err {};
	/** Exception for a socket connection failure. Usually this is thrown when
	 *  reading or writing. */
	class pop_socket_err : public pop_err {};
	/** Exception for a problem with a POP3 command. This exception may be
	 *  thrown in the following situations:
	 *  \begin{itemize}
	 *     \item There is an error when creating the line that is to be sent to
	 *           the server
	 *     \item There is an unexpected response by the server to the command
	 *     \item The command is not responded by OK
	 *  \end{itemize} */
	class pop_command_err : public pop_err {};
	/// This exception is thrown when a DoS attack is suspected. 
	class pop_dos_err : public pop_err {};
	/** This exception is thrown when login because the user provides no
	 *  password. */
	class pop_nologin_err : public pop_err {};


	// ========================================================================
	//  main
	// ========================================================================

	virtual void threaded_start (guint delay = 0);
	void start (void) throw (pop_err);
	void fetch (void) throw (pop_err);
	virtual void connect (void) throw (pop_err);
	void fetch_mails (gboolean statusonly = false) throw (pop_err);

 protected:
	void command_quit (void) throw (pop_err);
	guint command_stat (void) throw (pop_err);
	void command_top (std::vector<std::string> &, guint) throw (pop_err);
	void command_uidl (guint, std::map<guint,std::string> &) throw (pop_err);
	std::string command_uidl (guint) throw (pop_err);
	gint readline (std::string &, gboolean print=true, gboolean check=true,
				   gboolean checkline=true) throw (pop_err);
	gint sendline (const std::string, gboolean print=true, gboolean check=true)
				   throw (pop_err);
};

#endif
