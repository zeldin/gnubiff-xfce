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
// Author(s)     : Nicolas Rougier
// Short         : 
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <utime.h>

#include "mh.h"

// ========================================================================
//  base
// ========================================================================	
Mh::Mh (Biff *biff) : Local (biff)
{
	value ("protocol", PROTOCOL_MH);
}

Mh::Mh (const Mailbox &other) : Local (other)
{
	value ("protocol", PROTOCOL_MH);
}

Mh::~Mh (void)
{
}


// ========================================================================
//  base
// ========================================================================	
int
Mh::connect (void)
{
	// Build filename (.mh_sequences)
	gchar *filename = g_build_filename (address().c_str(), ".mh_sequences",
										NULL);
	if (!filename)
		return false;

	// Open file
	std::ifstream file;
	file.open (filename);
	g_free (filename);
	if (!file.is_open ())
		return false;

	// Parse mh sequences and try to find unseen sequence
	saved_.clear();
	while (!file.eof()) {
		std::string line;
		getline (file, line);

		// Got it!
		if (line.find("unseen:") == 0) {
			line = line.substr (7); // size of "unseen:" is 7
			if (!numbersequence_to_vector (line, saved_))
				return false;
			break;
		}
	}

	// Close file
	file.close();

	return true;
}

void
Mh::fetch (void)
{
	std::vector<std::string> mail;

	std::vector<guint> buffer = saved_;

	// Parse unseen sequence
	if (!connect()) {
		status (MAILBOX_ERROR);
		return;
	}

	// Get maximum number of mails to catch
	guint maxnum = INT_MAX;
	if (biff_->value_bool ("use_max_mail"))
		maxnum = biff_->value_uint ("max_mail");

	for (guint i=0; (i<saved_.size()) && (new_unread_.size() < maxnum); i++) {
		std::string line;
		std::ifstream file;    
		std::stringstream s;
		s << saved_[i];

		mail.clear();

		gchar *filename = g_build_filename (address().c_str(), s.str().c_str(),
											NULL);
		file.open (filename);
        g_free (filename);
		if (file.is_open()) {
			while (!file.eof()) {
				getline(file, line);
				mail.push_back(line);
			}
			parse (mail);
			file.close();
		}
	}
}
