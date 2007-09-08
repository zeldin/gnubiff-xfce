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
// Short         : Support functions
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#ifndef __SUPPORT_H__
#define __SUPPORT_H__

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif
#include "nls.h"


#include <glib.h>
#include <sstream>
#include <string>
#include <vector>

/**
 * This class provides various support functions.
 *
 * @see In the class Decoding are support functions for decoding, encoding
 *      and converting.
 */
class Support
{
protected:
	// Basic string functions
	gchar *utf8_strndup (const gchar *str, gsize n);

	// Advanced string functions
	std::string substitute (std::string format, std::string chars,
				std::vector<std::string> toinsert);
	gboolean numbersequence_to_vector (const std::string &seq,
				std::vector<guint> &vec, gboolean empty = true, char sep = ',',
				char range = '-');
	template<class Iter> std::string vector_to_numbersequence (Iter start,
				Iter end, const std::string sep = std::string (", "),
				const std::string range = std::string ("-"));
public:
	static guint version_to_integer (std::string versionstr,
									 gchar sep = '.');

	// File functions
	static std::string add_file_to_path (const std::string &path,
				const std::string file);
	static std::string path_get_basename (const std::string &path);
	static std::string path_get_dirname (const std::string &path);

	// Debugging
	static void unknown_internal_error_ (const gchar *file, guint line,
										 const gchar *func,
										 const gchar *signal);
};

/**
 *  Convert a sequence of unsigned integers (given by a start and end
 *  iterator) to a string. Consecutive numbers will be combined into a
 *  range expression.
 *
 *  Example: [1,2,3,5,2,3,4,5,17,17] gives "1-3, 5, 2-5, 17, 17"
 *
 *  @param  start  Start iterator
 *  @param  end    End iterator
 *  @param  sep    String to be used for separating list entries (default is
 *                 ", ")
 *  @param  range  String to be used for ranges (default is "-")
 *  @return        Sequence of numbers in a string
 */
template<class Iter> std::string 
Support::vector_to_numbersequence (Iter start, Iter end, const std::string sep,
								   const std::string range)
{
	std::stringstream result;
	guint num = 0, last = 0, inf_bound = 0;
	Iter pos = start;

	while (pos != end) {
		// Next number in sequence
		num = *(pos++);

		// No number stored
		if (last == 0) {
			inf_bound = last = num;
			if (pos != end)
				continue;
		}

		// Number is last number plus 1
		if (last+1 == num) {
			last++;
			if (pos != end)
				continue;
		}

		// We now have to put the numbers into the string
		if (result.str().size() > 0)
			result << sep;
		result << inf_bound;

		// Range?
		if (last > inf_bound)
			result << range << last;

		// Save new number
		inf_bound = last = num;
	}

	return result.str ();
}

/**
 *  Print debug information. This function should be called in a situation
 *  that should never happen to provide more information in a bug report.
 */
#define unknown_internal_error() (Support::unknown_internal_error_ (__FILE__, __LINE__, __func__, ""))

#endif
