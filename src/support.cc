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
// Short         : Functions that should be present in glib;-)
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#include <sys/utsname.h>
#include <glib.h>
#include <nls.h>
#include <sstream>
#include <string>
#include <vector>

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
gb_utf8_strndup(const gchar *str, gsize n)
{
    // No String
	if (str==NULL)
		return NULL;

	// Find the first character not to be copied
	gsize i=0;
	const gchar *lastpos=str;
	while ((i++<n)&&(*lastpos))
		lastpos=g_utf8_next_char(lastpos);

	gsize len=lastpos-str;
	return g_strndup(str,MAX(len,n));
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
gb_substitute(std::string format, std::string chars,
			  std::vector<std::string> toinsert)
{
	guint pos=0,cpos,prevpos=0;
	guint len=format.length();
	std::string result("");

	while ((pos<len)&&(pos=format.find("%",prevpos))!=std::string::npos)
    {
		if (prevpos<pos)
			result.append(format,prevpos,pos-prevpos);
		prevpos=pos+2;
		// '%' at end of string
		if (pos+1==len)
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
 *  An unknown internal error has been encountered. Print a message that
 *  asks the user to send a bug report. Print some additional information
 *  that may be of use.
 *
 *  @param file  Source file in which the error is
 *  @param line  Source line in which the error is
 *  @param func  Name of the function in which the error is
 */
void 
unknown_internal_error_(const gchar *file, guint line, const gchar *func)
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
	ss << _("You just found an unknown internal error. Please send a bug "
			"report to \"gnubiff-bugs@lists.sourceforge.net\".\n\n"
			"Additional information:\n");
	ss << "file        : " << file << "\n";
	ss << "line        : " << line << "\n";
	ss << "function    : " << func << "\n";
	ss << "date        : " << __DATE__ << " " << __TIME__ << "\n";
	ss << "gnubiff     : " << PACKAGE_VERSION << "\n";
	ss << "\n";
	ss << "system      : " << uts.sysname << " " << uts.release << " ";
	ss <<                     uts.version << " " << uts.machine << "\n";
	ss << "glib        : " << glib_major_version << "." << glib_minor_version;
	ss <<                     "." << glib_micro_version << "  ,  ";
	ss <<                     GLIB_MAJOR_VERSION << "." << GLIB_MINOR_VERSION;
	ss <<                     "." << GLIB_MICRO_VERSION << "\n";

	// Print error message
	g_warning (ss.str().c_str());
}
