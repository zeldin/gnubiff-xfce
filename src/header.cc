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

#include <glib.h>
#include <string>
#include <sstream>
#include "header.h"

/**
 *  Test whether two headers belong to the same mail.
 *
 *  @param   other  Reference to another header
 *  @returns        True if the two mails are equal, false otherwise
 */
bool 
Header::operator == (const Header &other) const
{
	return (mailid_ == other.mailid_);
}

/**
 *  Setting the gnubiff mail identifier for this mail header. If a unique
 *  identifier {\em uid} is provided it is taken. Otherwise (i.e.
 *  {\em uid} is an empty string) it is created by concatenating hash
 *  values of the sender, subject and date.
 *
 *  @param uid Unique identifier for the mail as provided by the protocol
 *             (POP3 and IMAP4) or an empty string.
 */
void 
Header::mailid (std::string uid = std::string(""))
{
	if (uid.size () > 0)
		mailid_ = uid;
	else {
		std::stringstream ss;
		ss << g_str_hash (sender.c_str());
		ss << g_str_hash (subject.c_str());
		ss << g_str_hash (date.c_str());
		mailid_ = ss.str ();
	}
}
