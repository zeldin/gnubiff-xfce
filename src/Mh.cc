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
#include <sys/stat.h>
#include <utime.h>
#include "Mh.h"


// ================================================================================
//  Constructors & Destructors
// --------------------------------------------------------------------------------
//  
// ================================================================================
Mh::Mh (Biff *owner) : Mailbox (owner)
{
	_protocol = PROTOCOL_MH;
}

Mh::Mh (Mailbox *other) : Mailbox (other)
{
	_protocol = PROTOCOL_MH;
}

Mh::~Mh (void)
{
}


// ================================================================================
//  Open unseen sequence and store unseen mail number into_saved
// --------------------------------------------------------------------------------
//  
// ================================================================================
int Mh::connect (void)
{
	std::string filename;
	struct stat file_stat;

	// Check for mail directory
	if ((stat (_address.c_str(), &file_stat) != 0) || (!S_ISDIR(file_stat.st_mode)))
		return false;

	// Open .mh_sequences file

	// Check if address ends with a trailing '/'
	if (_address[_address.size()-1] != '/')
		filename = _address + "/.mh_sequences";
	else
		filename = _address + ".mh_sequences";

	std::ifstream file;
	file.open (filename.c_str());
	if (!file.is_open())
		return false;

	//  Parse mh sequences and try to find unseen sequence
	while (!file.eof()) {
		std::string line;
		getline (file, line);

		// Got it !
		if (line.find("unseen:") != std::string::npos) {
			_saved.clear();

			// Analyze of the unseen sequence  whick looks like:
			//  unseen: 1-3, 7, 9-13, 16, 19
			guint i = line.find("unseen:")+std::string("unseen:").size();

			// Start parsing line looking for digit, range indicator, number separator
			guint inf_bound = 0;
			guint sup_bound = 0;

			while (i<line.size()) {
				// Got a digit ?
				if (isdigit (line[i])) {
					do {
						inf_bound *= 10;
						inf_bound += (line[i]-'0');
						i++;
					} while ((i < line.size()) && (isdigit(line[i])));
				}
				// Range indicator ?
				else if (line[i] == '-') {
					i++;
					sup_bound = 0;
					do {
						sup_bound *= 10;
						sup_bound += (line[i]-'0');
						i++;
					} while ((i < line.size()) && (isdigit(line[i])));
					for (guint j=inf_bound; j<=sup_bound; j++)
						_saved.push_back (j);
					inf_bound = 0;
					sup_bound = 0;
				}
				// End of a number
				else {
					if (inf_bound > 0) {
						_saved.push_back (inf_bound);
						inf_bound = 0;
					}
					i++;
				}	    
			}
			if (inf_bound > 0)
				_saved.push_back (inf_bound);
		}
	}

	// Close file
	file.close();

	return true;
}


// ================================================================================
//  Try to quickly get mailbox status
// --------------------------------------------------------------------------------
//
// ================================================================================
void Mh::get_status (void)
{
	std::vector<guint> buffer = _saved;

	_status = MAILBOX_CHECKING;

	// Parse unseen sequence
	if (!connect()) {
		_status = MAILBOX_ERROR;
		return;
	}


	// Find mailbox status by comparing saved list with the new one
	if (_saved.empty()) {
		_status = MAILBOX_EMPTY;
	}
	else if (buffer == _saved) {
		_status = MAILBOX_OLD;
	}
	else {
		_status = MAILBOX_OLD;
		guint i, j;
		for (i=0; i<_saved.size(); i++) {
			for (j=0; j<buffer.size(); j++) {
				if (_saved[i] == buffer[j])
					break;
			}
			if (j == buffer.size()) {
				_status = MAILBOX_NEW;
				break;
			}
		}
	}     
}


// ================================================================================
//  Get headers
// --------------------------------------------------------------------------------
//  
// ================================================================================
void Mh::get_header (void)
{
	std::vector<std::string> mail;

	std::vector<header> old_unread = _unread;
	_unread.clear();
	_seen.clear();
	for (guint i=0; (i<_saved.size()) && (_unread.size() < (unsigned int)( _owner->_max_collected_mail)); i++) {
		std::string line;
		std::ifstream file;    
		std::stringstream s;
		s << _saved[i];

		std::string filename;
		if (_address[_address.size()-1] != '/')
			filename = _address + std::string("/") + s.str();
		else
			filename = _address + s.str();
		mail.clear();
		file.open (filename.c_str());
		if (file.is_open()) {
			while (!file.eof()) {
				getline(file, line);
				mail.push_back(line);
			}
			parse (mail);
			file.close();
		}
	}

	if ((_unread == old_unread) && (_unread.size() > 0))
		_status = MAILBOX_OLD;
}
