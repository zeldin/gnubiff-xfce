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
// Author(s)     : Nicolas Rougier, Robert Sowada
// Short         : All information about a specific mail needed by gnubiff
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#ifndef __HEADER_H__
#define __HEADER_H__

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

/**
 *  All the information about a specific mail needed by gnubiff. Headers are
 *  referred to by the mailid.
 */
class Header {
public:
	/// Sender of the mail
	std::string		sender;
	/// Subject of the mail
	std::string		subject;
	/// Date of the mail
	std::string		date;
	/// First lines of the mail' body
	std::string		body;
	/// Characterset of the mail's body
	std::string		charset;
	/// Position in the mailbox
	guint			position_;
protected:
	/**
	 *  This is a (hopefully) unique identifier for the mail. If supported by
	 *  the protocol this will be the unique id of the mail that is provided
	 *  by the server (this is the case for POP3 and IMAP4). Otherwise
	 *  gnubiff creates an own identifier.
	 *
	 *  Remark: This identifier must not contain whitespace characters!
	 *
	 *  @see The mail identifier is calculated by the method Header::mailid().
	 */
	std::string mailid_;

public:
	bool operator == (const Header &other) const;
	void mailid (std::string uid);

	/// Access function to Header::mailid_
	const std::string mailid (void) {return mailid_;}
};

#endif
