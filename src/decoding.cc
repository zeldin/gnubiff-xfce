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
 * Decoding of a quoted-printable encoded string. This string must consist of
 * exactly one line (there is no handling of soft breaks etc., see
 * RFC 2045 6.7. (3)-(5). If the given string {\em todec} is not valid an
 * empty string is returned.
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
