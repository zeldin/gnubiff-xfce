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
// Author(s)     : Nicolas Rougier, Robert Sowada
// Short         : Support functions
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#include <sys/utsname.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <nls.h>
#include <sstream>
#include <string>
#include <vector>
#include "support.h"

/** 
 * Duplicates the first {\em n} characters of a valid utf-8 character array,
 * returning a newly-allocated buffer {\em n + 1} characters long which will
 * always be nul-terminated. If {\em str} is less than {\em n} characters long
 * the buffer is padded with nuls. If {\em str} is NULL it returns NULL. The
 * returned value should be freed when no longer needed.
 *
 * @param  str  the valid utf-8 character array to duplicate
 * @param  n    the maximum number of utf-8 characters to copy from {\em str}
 * @return      a newly-allocated buffer containing the first {\em n}
 *              characters of {\em str}, nul-terminated
 */
gchar* 
Support::utf8_strndup (const gchar *str, gsize n)
{
    // No String
	if (str == NULL)
		return NULL;

	// Find the first character not to be copied
	gsize i = 0;
	const gchar *lastpos = str;
	while ((i++ < n) && (*lastpos))
		lastpos = g_utf8_next_char (lastpos);

	gsize len = lastpos-str;
	return g_strndup (str, MAX (len,n));
}

/**
 * Similar to the printf function all '%'-sequences in {\em format} are
 * substituted with strings of {\em toinsert}. A '%' at the end of {\em format}
 * is erased, '%%' leads to '%' in the return value. The sequence '%x' is
 * erased if character 'x' is no element of {\em chars}, otherwise it is
 * replaced by a string of the array {\em toinsert}: If 'x' is the {\em n}-th
 * character in {\em chars} it is substituted with the {\em n}-th string of
 * {\em toinsert} (if there are multiple occurances of 'x' in {\em chars} the
 * first is taken).
 *
 * If all strings are valid utf-8 strings and all characters in {\em chars}
 * are single byte utf-8 characters the return value will be a valid utf-8
 * string.
 *
 * @param  format   the format string
 * @param  chars    the string made up of characters to be substituted in
 *                  {\em format} when following a '%'
 * @param  toinsert the array of strings to be inserted into the format string
 *                  {\em format}
 * @return          format string {\em format} with all '%'-sequences
 *                  substituted
 */
std::string 
Support::substitute(std::string format, std::string chars,
					std::vector<std::string> toinsert)
{
	std::string::size_type pos = 0, cpos, prevpos=0;
	std::string::size_type len = format.length();
	std::string result("");

	while ((pos < len)
		   && (pos = format.find("%", prevpos)) != std::string::npos) {
		if (prevpos < pos)
			result.append (format, prevpos, pos-prevpos);
		prevpos = pos+2;
		// '%' at end of string
		if (pos+1 == len)
			return result;

		// '%%'
		if (format[pos+1] == '%')
		{
			result+='%';
			continue;
		}

		// generic case
		if ((cpos = chars.find(format[pos+1])) == std::string::npos)
			continue;
		result += toinsert[cpos];
	}
	if (prevpos<len)
		result.append (format, prevpos, len-prevpos);
	return result;
}

/**
 *  Convert sequence of strictly positive numbers (given as a string) into a
 *  vector. The sequence is to be given as a {\em sep} separated list of
 *  numbers and ranges of numbers.  Whitespace and newline characters are
 *  ignored (unless the {\em sep} or {\em range} are such a character).
 *
 *  Example: "1, 5, 6-9, 12, 4-1, 3, 5" gives [1,5,6,7,8,9,12,3,5]
 *
 *  @param  seq    List of numbers and ranges to convert
 *  @param  vec    Vector in which the numbers of the sequence are returned
 *                 if the return value is true, otherwise the value of
 *                 {\em vec} will be undetermined
 *  @param  empty  If true the vector will be cleared before parsing the
 *                 sequence (the default is true)
 *  @param  sep    Character to be used for separating list entries (default
 *                 is ',')
 *  @param  range  Character to be used for ranges (default is '-')
 *  @return        Boolean indicating success
 */
gboolean 
Support::numbersequence_to_vector (const std::string &seq,
								   std::vector<guint> &vec, gboolean empty,
								   char sep, char range)
{
	std::string::size_type len = seq.length(), pos = 0;
	guint inf_bound = 0, sup_bound = 0;

	// Clear vector if wished by the user
	if (empty)
		vec.clear ();

	while (pos < len) {
		char c = seq[pos++];

		// Got a digit?
		if (g_ascii_isdigit (c)) {
			// Do we already have a number?
			if (sup_bound > 0)
				return false;
			// Get number
			sup_bound = c - '0';
			while ((pos < len) && (g_ascii_isdigit (seq[pos])))
				sup_bound = 10*sup_bound + (seq[pos++] - '0');
			continue;
		}

		// Range indicator?
		if (c == range) {
			// Test for "num-num-num" format error
			if (inf_bound > 0)
				return false;
			inf_bound = sup_bound;
			sup_bound = 0;
			continue;
		}

		// Separator?
		if (c == sep) {
			// No number at all or no end of range given
			if (sup_bound == 0)
				return false;
			// Convert single number to range
			if (inf_bound == 0)
				inf_bound = sup_bound;
			// Add numbers to vector
			for (guint i = inf_bound; i <= sup_bound; i++)
				vec.push_back (i);
			inf_bound = 0;
			sup_bound = 0;
			continue;
		}

		// Ignore whitespace and newlines
		if ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r'))
			continue;

		// Other character
		return false;
	}

	// Add last number/range
	if (sup_bound == 0)
		return true;
	// Convert single number to range
	if (inf_bound == 0)
		inf_bound = sup_bound;
	// Add numbers
	for (guint i = inf_bound; i <= sup_bound; i++)
		vec.push_back (i);
	return true;
}

/**
 *  Add the name of a file (or directory) to the given path {\em path}.
 *
 *  @param  path  Path to which {\em file} will be added.
 *  @param  file  Name of the file (or directory) that will be added to
 *                {\em path}.
 *  @return       Resulting path or the empty string in case of an error.
 */
std::string 
Support::add_file_to_path (std::string path, std::string file)
{
	std::string result = std::string ("");
	gchar *filename = g_build_filename (path.c_str(), file.c_str(), NULL);
	if (filename)
		result = std::string (filename);
	g_free (filename);
	return result;
}

/**
 *  An unknown internal error has been encountered. Print a message that
 *  asks the user to send a bug report. Print some additional information
 *  that may be of use.
 *
 *  @param file  Source file in which the error is
 *  @param line  Source line in which the error is
 *  @param func  Name of the function in which the error is
 */
void 
Support::unknown_internal_error_ (const gchar *file, guint line,
								  const gchar *func)
{
	std::stringstream ss;
	utsname uts;

	// Get system information
	if (uname (&uts) < 0) {
		uts.sysname[0] = '\0';
		uts.release[0] = '\0';
		uts.version[0] = '\0';
		uts.machine[0] = '\0';
	}

	// Create error message
	ss << _("You just found an unknown internal error. Please send a detailed "
			"bug report to \"gnubiff-bugs@lists.sourceforge.net\".\n\n"
			"Additional information:\n");
	ss << "file        : " << file << "\n";
	ss << "line        : " << line << "\n";
	ss << "function    : " << func << "\n";
	ss << "date        : " << __DATE__ << " " << __TIME__ << "\n";
	ss << "gnubiff     : " << PACKAGE_VERSION << " ";
	ss <<                     IS_CVS_VERSION ? "CVS\n" : "\n";
	ss << "\n";
	ss << "system      : " << uts.sysname << " " << uts.release << " ";
	ss <<                     uts.version << " " << uts.machine << "\n";
	ss << "sizeof      : " << "gint=" << sizeof (gint) << " ";
	ss <<                     "gsize=" << sizeof (gsize) << " ";
	ss <<                     "s:s:s_t="<<sizeof(std::string::size_type)<<"\n";
	ss << "glib        : " << glib_major_version << "." << glib_minor_version;
	ss <<                     "." << glib_micro_version << " (dyn),  ";
	ss <<                     GLIB_MAJOR_VERSION << "." << GLIB_MINOR_VERSION;
	ss <<                     "." << GLIB_MICRO_VERSION << " (stat)\n";
	ss << "gtk         : " << gtk_major_version << "." << gtk_minor_version;
	ss <<                     "." << gtk_micro_version << " (dyn),  ";
	ss <<                     GTK_MAJOR_VERSION << "." << GTK_MINOR_VERSION;
	ss <<                     "." << GTK_MICRO_VERSION << " (stat)\n";

	// Print error message
	g_warning (ss.str().c_str());
}
