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
// Short         : Various functions for decoding, converting ...
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#include "decoding.h"
#include "nls.h"

/** 
 * Decodes the body of a mail.
 * The part of the mail's body that will be displayed by gnubiff is decoded.
 * The encoding is given by the parameter {\em encoding} and must be obtained
 * before. Currently supported encodings are 7bit, 8bit and quoted-printable.
 * If called with an unsupported encoding the mail's body is replaced with an
 * error message.
 *
 * @param  mail      C++ vector of C++ strings consisting of the lines of the
 *                   mail.
 * @param  encoding  C++ string for the encoding of the mail's body.
 * @return           Boolean indicating success.
 */
gboolean 
Decoding::decode_body (std::vector<std::string> &mail, std::string encoding)
{
	// Skip header
	guint bodypos=0;
	while ((bodypos<mail.size()) && (!mail[bodypos].empty()))
		bodypos++;
	bodypos++;

	// 7bit, 8bit encoding: nothing to do
	if ((encoding=="7bit") || (encoding=="8bit")); // || (encoding=="binary"));
	// Quoted-Printable
	else if (encoding=="quoted-printable") {
		std::vector<std::string> decoded=decode_quotedprintable(mail, bodypos);
		mail.erase(mail.begin()+bodypos, mail.end());
		for (guint i=0; i<decoded.size(); i++)
			mail.push_back(decoded[i]);
	}
	// Unknown encoding: Replace body text by a error message
	else {
		mail.erase(mail.begin()+bodypos, mail.end());
		gchar *tmp=g_strdup_printf(_("[The encoding \"%s\" of this mail can't be decoded]"),encoding.c_str());
		mail.push_back(std::string(tmp));
		g_free(tmp);
		return true;
	}

	return true;
}

/**
 * Decoding of a base64 encoded string. If the given string
 * {\em todec} is not valid an empty string is returned.
 * See RFC 3548 for definition of base64 encoding.
 *
 * @param  todec  Reference to a C++ string to be decoded
 * @return        C++ string consisting of the decoded string {\em todec}
 */
std::string 
Decoding::decode_base64 (const std::string &todec)
{
	static const int index_64[128] = {
		-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
		52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-1,-1,-1,
		-1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
		15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
		-1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
		41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
	};
#   define BASE64(c) (index_64[(unsigned char)(todec[c]) & 0x7F])
	std::string result;
	guint pos=0, len=todec.length();

	if (len%4!=0)
		return std::string("");

	while (pos+3<len) {
		if ((todec[pos]&0x80) || (todec[pos+1]&0x80) || (todec[pos+2]&0x80)
			|| (todec[pos+3]&0x80) || (index_64[(int)todec[pos]]<0)
			|| (index_64[(int)todec[pos+1]]<0))
			return std::string("");
		result+=(char)((BASE64(pos) << 2) | (BASE64(pos+1) >> 4));
		if (todec[pos+2]=='=')
			if ((todec[pos+3]=='=') && (pos+4==len) && (!(BASE64(pos+1)&15)))
				return result;
			else
				return std::string("");
		if (index_64[(int)todec[pos+2]]<0)
			return std::string("");
		result+=(char)(((BASE64(pos+1) & 0xf) << 4) | (BASE64(pos+2) >> 2));
		if (todec[pos+3]=='=')
			if ((pos+4==len) && (!(BASE64(pos+2)&3)))
				return result;
			else
				return std::string("");
		if (index_64[(int)todec[pos+3]]<0)
			return std::string("");
		result+=(char)(((BASE64(pos+2) & 0x3) << 6) | BASE64(pos+3));
		pos+=4;
	}
	return result;
}

/**
 * Decoding of a q-encoded strings. Q-Encoding is similar to quoted-printable
 * encoding and is used in mail headers. See RFC 2047 4.2. for a definition.
 * If the given string {\em todec} is not valid an empty string is returned.
 *
 * @param  todec  Reference to a C++ string to be decoded
 * @return        C++ string consisting of the decoded string {\em todec}
 */
std::string 
Decoding::decode_qencoding (const std::string &todec)
{
	guint pos=0,len=todec.length();
	std::string result;
	gint decoded;

	while (pos<len)
	{
		switch (gchar c=todec.at(pos++))
		{
			case '=':
				pos+=2;
				if (pos>len)
					return result;
				if ((decoded=g_ascii_xdigit_value(todec.at(pos-1)))<0)
					return std::string("");
				if ((decoded+=16*g_ascii_xdigit_value(todec.at(pos-2)))<0)
					return std::string("");
				result+=decoded;
				break;
			case '_':
				result+=' ';
				break;
			default:
				result+=c;
				break;
		}
	}
	return result;
}

/**
 * Decoding of a quoted-printable encoded string. This string must consist of
 * exactly one line (there is no handling of soft breaks etc., see
 * RFC 2045 6.7. (3)-(5)). If the given string {\em todec} is not valid an
 * empty string is returned.
 *
 * Note: For mail headers q-encoding is used instead of quoted-printable.
 *
 * @param  todec  Reference to a C++ string to be decoded
 * @return        C++ string consisting of the decoded string {\em todec}
 */
std::string 
Decoding::decode_quotedprintable (const std::string &todec)
{
	guint pos=0,len=todec.length();
	std::string result;
	gint decoded;

	while (pos<len)
	{
		switch (gchar c=todec.at(pos++))
		{
			case '=':
				pos+=2;
				if (pos>len)
					return result;
				if ((decoded=g_ascii_xdigit_value(todec.at(pos-1)))<0)
					return std::string("");
				if ((decoded+=16*g_ascii_xdigit_value(todec.at(pos-2)))<0)
					return std::string("");
				result+=decoded;
				break;
			default:
				result+=c;
				break;
		}
	}
	return result;
}

/**
 * Decoding of a vector of quoted-printable encoded strings. It is assumed that
 * each string in the vector {\em todec} represents one line of the
 * quoted-printable encoded text. Lines that have an invalid encoding are
 * omitted.
 *
 * See RFC 2045 6.7. for the definition of this encoding.
 *
 * Note: For mail headers q-encoding is used instead of quoted-printable.
 *
 * @param todec  Reference to a C++ vector of C++ strings that will be decoded.
 * @param pos    Number of the first line in the vector that has to be decoded.
 *               The default value is 0.
 * @return       C++ vector of C++ strings consisting of the decoded text
 */
std::vector<std::string> 
Decoding::decode_quotedprintable (const std::vector<std::string> &todec,
								  guint pos)
{
	std::string line;
	std::vector<std::string> result;

	while (pos<todec.size()) {
		// Handle soft breaks (see RFC 2045 6.7. (3),(5))
		line+=todec[pos];
		gint lpos=line.size()-1;
		while ((lpos>=0) && ((line[lpos]=='\t') || (line[lpos]==' ')))
			lpos--;
		if (lpos<(signed)line.size()-1)
			line.erase(lpos+1,line.size());
		if ((line.size()>0) && (line[line.size()-1]=='=')) {
			line.erase(line.size()-1);
			if (pos<todec.size()-1) {
				pos++;
				continue;
			}
		}
		// Decode line
		result.push_back(decode_quotedprintable(line));
		line="";
		pos++;
	}
	return result;
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
Decoding::utf8_to_imaputf7(const gchar *str, gssize len)
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
