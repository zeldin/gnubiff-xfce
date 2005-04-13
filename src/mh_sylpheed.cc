// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2005 Nicolas Rougier
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
// Author(s)     : Robert Sowada, Nicolas Rougier
// Short         : Mh protocol as used by Sylpheed
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#include <fstream>
#include <sstream>
#include "mh_sylpheed.h"
#include "nls.h"

// ========================================================================
//  base
// ========================================================================	
/**
 * Constructor. The local mailbox for the mh protocol (as used by sylpheed)
 * is created from scratch.
 *
 * @param biff Pointer to the instance of Gnubiff.
 */
Mh_Sylpheed::Mh_Sylpheed (Biff *biff) : Mh_Basic (biff)
{
	value ("protocol", PROTOCOL_MH_SYLPHEED);
}

/**
 * Constructor. The local mailbox for the mh protocol (as used by sylpheed)
 * is created by taking the attributes of the existing mailbox {\em other}.
 *
 * @param other Mailbox from which the attributes are taken.
 */
Mh_Sylpheed::Mh_Sylpheed (const Mailbox &other) : Mh_Basic (other)
{
	value ("protocol", PROTOCOL_MH_SYLPHEED);
}

/// Destructor
Mh_Sylpheed::~Mh_Sylpheed (void)
{
}

// ========================================================================
//  main
// ========================================================================	

/**
 *  Get message numbers of the mails to be parsed. The message numbers of
 *  unread mails are stored in the file ".sylpheed_mark" by Sylpheed.
 *
 *  @param  msn    Reference to a vector in which the message numbers are
 *                 returned
 *  @param  empty  Whether the vector shall be emptied before obtaining the
 *                 message numbers (the default is true)
 *  @exception local_file_err
 *                 This exception is thrown when the file ".sylpheed_mark"
 *                 could not be opened.
 *  @exception local_info_err
 *                 This exception is thrown when the ".sylpheed_mark" file
 *                 can't be parsed successfully.
 */
void 
Mh_Sylpheed::get_messagenumbers (std::vector<guint> &msn, gboolean empty)
									throw (local_err)
{
	// Empty the vector if wished for
	if (empty)
		msn.clear ();

	// Open file
	std::string filename = add_file_to_path (address (), ".sylpheed_mark");
	std::ifstream file;
	file.open (filename.c_str ());
	if (!file.is_open ()) throw local_file_err ();
	if (file.eof()) throw local_info_err();

	// Get version of file
	guint32 version;
	file.read ((char *)&version, sizeof(version));
	if (version != 2) {
		g_warning (_("Version \"%u\" of sylpheed mark file not supported"),
					 version);
		throw local_info_err();
	}

	// Read message numbers
	while (true) {
		guint32 mn, flags;
		file.read ((char *)&mn, sizeof(mn)).read ((char *)&flags, sizeof(mn));
		if (file.eof())
			break;

		// (MSG_NEW || MSG_UNREAD) && !MSG_DELETED (see sylpheed sourcefile
		// "src/procmsg.h")
		if ((flags & 3) && !(flags & 8))
			msn.push_back (mn);
	}

	// Close file
	file.close();
}

/**
 *  Give the name of the file that shall be monitored by FAM. For the mh
 *  protocol this is the ".mh_sequences" file.
 *
 *  @return    Name of the file to be monitored.
 */
std::string 
Mh_Sylpheed::file_to_monitor (void)
{
	return add_file_to_path (address(), std::string (".sylpheed_mark"));
}
