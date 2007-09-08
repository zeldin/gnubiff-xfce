// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2007 Nicolas Rougier, 2004-2007 Robert Sowada
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

#include <fstream>
#include <sstream>
#include "mh.h"

// ========================================================================
//  base
// ========================================================================	
/**
 * Constructor. The local mailbox for the mh protocol is created from
 * scratch.
 *
 * @param biff Pointer to the instance of Gnubiff.
 */
Mh::Mh (Biff *biff) : Mh_Basic (biff)
{
	value ("protocol", PROTOCOL_MH);
}

/**
 * Constructor. The local mailbox for the mh protocol is created by
 * taking the attributes of the existing mailbox {\em other}.
 *
 * @param other Mailbox from which the attributes are taken.
 */
Mh::Mh (const Mailbox &other) : Mh_Basic (other)
{
	value ("protocol", PROTOCOL_MH);
}

/// Destructor
Mh::~Mh (void)
{
}

// ========================================================================
//  main
// ========================================================================	

/**
 *  Get message numbers of the mails to be parsed. In the mh protocol the
 *  message numbers of unread mails are stored in the file ".mh_sequences".
 *
 *  @param  msn    Reference to a vector in which the message numbers are
 *                 returned
 *  @param  empty  Whether the vector shall be emptied before obtaining the
 *                 message numbers (the default is true)
 *  @exception local_file_err
 *                 This exception is thrown when the file ".mh_sequences"
 *                 could not be opened.
 *  @exception local_info_err
 *                 This exception is thrown when the ".mh_sequences" file
 *                 can't be parsed successfully.
 */
void 
Mh::get_messagenumbers (std::vector<guint> &msn, gboolean empty)
						throw (local_err)
{
	// Empty the vector if wished for
	if (empty)
		msn.clear ();

	// Open file
	std::string filename = add_file_to_path (address (), ".mh_sequences");
	std::ifstream file;
	file.open (filename.c_str ());
	if (!file.is_open ()) throw local_file_err ();

	// Parse mh sequences and try to find the unseen sequence
	std::string line;
	getline (file, line);
	while (!file.eof()) {
		// Got it!
		if (line.find ("unseen:") == 0) {
			line = line.substr (7); // size of "unseen:" is 7
			if (!numbersequence_to_vector (line, msn)) throw local_info_err();
			break;
		}
		// Read next line
		getline (file, line);
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
Mh::file_to_monitor (void)
{
	return add_file_to_path (address(), std::string(".mh_sequences"));
}
