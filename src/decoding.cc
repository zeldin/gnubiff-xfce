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
// Short         : Various functions for decoding, converting ...
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#include <sstream>
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
 *  Get a quoted string that is a substring of the string {\em line}. The
 *  quoted string has to be enclosed by the {\em quoted} character. If
 *  there should be no test for this character at the beginning {\em
 *  test_start} has to be false, if it is okay that the quoted string does
 *  not end with the {\em quoted} character but with the end of {\em
 *  line}. The position {\em pos} points to the first character of the
 *  quoted string. If the quoted string can be successfully obtained it is
 *  returned in {\em str} (with all quoted pairs "\x" substituted by "x"
 *  for any character 'x') and {\em pos} is the position of the next
 *  character of {\em line} after the quoted string. If {\em line} ends with
 *  the quoted string {\em pos} will point outside of {\em line}!
 *  
 *  @param  line       String in which the quoted string is contained
 *  @param  str        Here the obtained string is returned
 *  @param  pos        Position of the first character of the quoted string.
 *                     When returning {\em pos} is the position of the next
 *                     character after the quoted string. If false is returned
 *                     it is the position in which the error occurred.
 *  @param  quoted     Character that encloses the quoted string (default 
 *                     is '"')
 *  @param  test_start Shall the first character be tested for being the
 *                     {\em quoted} character (default is true)?
 *  @param  end_ok     Is it okay for the quoted string to end with the end of
 *                     {\em line} and not with {\em quoted} (default is false)?
 *  @return            Boolean indicating success
 */
gboolean 
Decoding::get_quotedstring (std::string line, std::string &str, guint &pos,
							gchar quoted, gboolean test_start, gboolean end_ok)
{
	guint len = line.size ();
	str = std::string ("");
	if (pos >= len)
		return false;

	if ((test_start) && (line[pos++] != quoted))
		return false;

	while ((pos < len) && (line[pos] != quoted)) {
		if ((line[pos] == '\\') && (pos+1 == len))
			return false;
		if (line[pos] == '\\')
			pos++;
		str += line[pos++];
	}

	if (pos == len)
		return end_ok;
	pos++;
	return true;
}

/**
 *  Get a token that is a substring of {\em line}. This token may only consist
 *  of those characters that are defined in RFC 2045 5.1.
 *  
 *  @param  line       String in which the quoted string is contained
 *  @param  str        Here the obtained string is returned
 *  @param  pos        Position of the first character of the token.
 *                     When returning {\em pos} is the position of the next
 *                     character after the token. If false is returned
 *                     it is the position in which the error occurred.
 *  @param  lowercase  Shall the token be converted to lower case (default is
 *                     true)?
 *  @return            Boolean indicating success
 */
gboolean 
Decoding::get_mime_token (std::string line, std::string &str, guint &pos,
						  gboolean lowercase)
{
	// Non alphanumeric characters allowed in tokens
	const static std::string token_ok = "!#$%&'*+-._`{|}~";

	guint len = line.size();
	while ((pos < len) && ((g_ascii_isalnum(line[pos]))
						   || (token_ok.find(line[pos]) != std::string::npos)))
		str += line[pos++];
	if (str.size() == 0)
		return false;
	if (lowercase)
		str = ascii_strdown (str);
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

/**
 *  Convert all upper case characters in an ASCII string to lower case
 *  characters.  Non-ASCII characters are left unchanged.
 *
 *  @param  str String to be converted
 *  @retrun     Converted string
 */
std::string 
Decoding::ascii_strdown (const std::string &str)
{
	gchar *tmp = g_ascii_strdown (str.c_str(), -1);
	std::string result = std::string (tmp);
	g_free (tmp);
	return result;
}

/**
 * Encrypt the password for saving.
 * Pop3 and Imap4 protocols require password in clear so we have to
 * save passwords in clear within the configuration file. No need to say
 * this is higly unsecure if somebody looks at the file. So we try to
 * take some measures:
 * \begin{itemize}
 *    \item The file is made readable by owner only.
 *	  \item Password is "crypted" so it's not directly human readable.
 * \end{\itemize}
 * Of course, these measures won't prevent a determined person to break
 * in but will at least prevent "ordinary" people to be able to steal
 * password easily.
 *
 * If no password saving is selected at configure time, an empty string is
 * returned.
 *
 * @param password  Password to be encrypted
 * @param passtable Passtable to be used for the encryption
 * @return          "Encrypted" password or empty string
 */
std::string 
Decoding::encrypt_password (const std::string &password,
							const std::string &passtable)
{
#ifdef USE_PASSWORD
	std::stringstream encrypted;

	for (guint j=0; j < password.size(); j++)
		encrypted << passtable[password[j]/16] << passtable[password[j]%16];
	return encrypted.str();
#else
	return std::string("");
#endif
}

/**
 * Decrypt a password from the config file.
 *
 * If no password saving is selected at configure time, an empty string is
 * returned.
 *
 * @param password  Password to be decrypted
 * @param passtable Passtable to be used for the decryption
 * @return          Decrypted password or empty string
 * @see             Decoding::crypt_password()
 */
std::string 
Decoding::decrypt_password (const std::string &password,
							const std::string &passtable)
{
#ifdef USE_PASSWORD
	std::stringstream decrypted;

	for (guint i = 0; i+1 < password.size(); i += 2) {
		char c = 0;
		guint j;
		for (j=0; j<16; j++) {
			if (passtable [j] == password[i])
				c += j*16;
			if (passtable [j] == password[i+1])
				c += j;
		}
		decrypted << c;
	}
	return decrypted.str();
#else
	return std::string("");
#endif
}
