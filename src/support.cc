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
// Short         : Functions that should be present in glib;-)
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#include <glib.h>
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
 * This function converts an utf-8 encoded character array to an imap modified
 * utf-7 character array. Unfortunately glib function g_convert() can only
 * convert to regular utf-7 (see RFC 2152) but IMAP needs a modified version
 * of utf-7 (see RFC 3501 5.1.3).
 *
 * If {\em len} is negative then {\em str} must be a nul-terminated valid utf-8
 * string and the whole string will be converted. If {\em len} is positive
 * {\em str} must contain at least {\em len} bytes (forming a valid utf-8
 * string) that will be converted. If the conversion is not successful NULL
 * will be returned, otherwise a newly allocated nul-terminated character array
 * containing the converted string will be returned. This array must be freed
 * using g_free().
 *
 * @param  str a valid utf-8 character array, nul-terminated if {\em len} is
 *             less than zero
 * @param  len number of characters of {\em str} that should be converted or
 *             less than zero if {\em str} is nul-terminated
 * @return     a newly allocated nul-terminated character array or NULL
 */
gchar* 
gb_utf8_to_imaputf7(const gchar *str, gssize len)
{
	// Modified base64 characters (see RFC 2045, RFC 3501 5.1.3)
	const char *modbase64="ABCDEFGHIJKLMNOPQRSTUVWXYZ"
						  "abcdefghijklmnopqrstuvwxyz0123456789+,";

	// No String or nothing to do
	if ((str==NULL) || (len==0))
		return NULL;

	gchar c=*str;;
	std::string result;
	gssize cnt_len=0;
	gboolean printableascii=true;
	const gchar *start = str;

	while (((len<0) && (*str!='\0')) || (cnt_len<len) || (!printableascii))
	{
		if (cnt_len!=len)
			c=*str;

		// End of non (printable) ASCII characters?
		if (((!printableascii) && (c>='\x20') && (c<='\x7e'))
			|| (((len<0) && (c=='\0')) || ((cnt_len>=len) && (len>0))))
		{
			result+='&';

			// Convert first to UTF-16
			gsize cnt=0;
			gchar *utf16=g_convert(start,str-start,"UTF-16BE","UTF-8",
								   NULL,&cnt,NULL);
			if (utf16==NULL)
				return NULL;
			
			// Convert to modified BASE64
			gchar *pos=utf16;
			while (cnt>0)
			{
				gchar b[5]="\0\0\0\0";
				b[0]=modbase64[(pos[0]&0xfc)>>2];
				if (cnt==1)
					b[1]=modbase64[(pos[0]&0x03)<<4];
				else
				{
					b[1]=modbase64[((pos[0]&0x03)<<4)|(pos[1]&0xf0)>>4];
					if (cnt==2)
						b[2]=modbase64[(pos[1]&0x0f)<<2];
					else
					{
						b[2]=modbase64[((pos[1]&0x0f)<<2)|(pos[2]&0xc0)>>6];
						b[3]=modbase64[pos[2]&0x3f];
					}
				}
				result+=b;
				if (cnt>2)
					cnt-=3;
				else
					cnt=0;
				pos+=3;
			}

			g_free(utf16);
			result+="-";
			printableascii=true;
			continue;
		}

		cnt_len++;
		str++;
		  
		// (Printable) ASCII character?
		if ((printableascii) && (c>='\x20') && (c<='\x7e'))
		{
			result+=c;
			if (c=='&')
				result+='-';
			continue;
		}

		// End of (printable) ASCII characters?
		if (printableascii)
		{
			printableascii=false;
			start=str-1;
			continue;
		}

		// Another non (printable) ASCII character!
	}

	return g_strdup(result.c_str());
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
		if (format.at(pos+1)=='%')
		{
			result+='%';
			continue;
		}

		// generic case
		if ((cpos=chars.find(format.at(pos+1)))==std::string::npos)
			continue;
		result+=toinsert.at(cpos);
	}
	if (prevpos<len)
		result.append(format,prevpos,len-prevpos);
	return result;
}
