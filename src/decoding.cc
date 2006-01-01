// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2006 Nicolas Rougier, 2004-2006 Robert Sowada
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
 * Decode the body of a mail.
 * The part of the mail's body that will be displayed by gnubiff is decoded.
 * The encoding is given by the parameter {\em encoding} and must be obtained
 * before. Currently supported encodings are 7bit, 8bit and quoted-printable.
 * If called with an unsupported encoding the mail's body is replaced with an
 * error message.
 *
 * @param  mail        Vector of strings consisting of the mail's lines.
 * @param  encoding    Encoding of the mail's body.
 * @param  bodypos     If the beginning of the mail's body is known (i.e.
 *                     {\em skip_header} is false) this is the position of the
 *                     first body line, otherwise it is the line in which the
 *                     search for the end of the header starts (default is 0)
 * @param  skip_header If {\em bodypos} is the first line of the body this has
 *                     to be false, otherwise true (default is true)
 * @return             Boolean indicating success.
 */
gboolean 
Decoding::decode_body (std::vector<std::string> &mail, std::string encoding,
					   std::string::size_type bodypos, gboolean skip_header)
{
	// If mail is empty nothing has to be decoded
	if (mail.size() == 0)
		return true;

	// Skip header
	if (skip_header) {
		while ((bodypos<mail.size()) && (!mail[bodypos].empty()))
			bodypos++;
		bodypos++;
	}

	// Invalid bodypos?
	if (bodypos >= mail.size())
		return true;

	// 7bit, 8bit encoding: nothing to do
	if ((encoding=="7bit") || (encoding=="8bit")); // || (encoding=="binary"));
	// Quoted-Printable
	else if (encoding=="quoted-printable") {
		std::vector<std::string> decoded=decode_quotedprintable(mail, bodypos);
		mail.erase(mail.begin()+bodypos, mail.end());
		for (std::string::size_type i = 0; i < decoded.size(); i++)
			mail.push_back(decoded[i]);
	}
	// Unknown encoding: Replace body text by a error message
	else {
		mail.erase (mail.begin()+bodypos, mail.end());
		gchar *tmp = g_strdup_printf (_("[The encoding \"%s\" of this mail "
										"can't be decoded]"),
									  encoding.c_str());
		if (tmp)
			mail.push_back (std::string(tmp));
		g_free (tmp);
		return false;
	}

	return true;
}

/**
 *  Decode a header line. Any quoted-printable or base64 encoding is
 *  decoded to utf-8. If there is an error during decoding an error message
 *  is returned.  Subject lines are kind of special because the character
 *  set is encoded within the text. For example it can be something like:
 *  =?iso-8859-1?Q?Apr=E8s?=.
 *
 *  @param  line Header line to be decoded
 *  @return      String containing the decoded line (or an error message).
 **/
std::string 
Decoding::decode_headerline (const std::string &line)
{
	// A mail header line (sender, subject or date) cannot contain
	// non-ASCII characters, so first we remove any non-ASCII characters
	std::string copy, result;
	std::string::size_type len = line.size();
	for (std::string::size_type i = 0; i < len; i++)
		if (line[i] >= 0)
			copy += line[i];
	len = copy.size();

	// Now we can begin decoding
	std::string::size_type i = 0;
	while (i < len) {
		// An encoded word (see RFC 2047)?
		std::string::size_type j = i;
		while ((j+1 < len) && (copy[j] == '=') && (copy[j+1] == '?')) {
			std::string charset, encoding, text, decoded;
			gchar *utf8;
			if (!parse_encoded_word (copy, charset, encoding, text, j))
				return _("[Cannot decode this header line]");
			// Decode and convert text
			if (encoding == "q")
				decoded = decode_qencoding (text);
			else if (encoding == "b")
				decoded = decode_base64 (text);
			else
				return _("[Cannot decode this header line]");
			if (decoded.size() > 0) {
				utf8 = g_convert (decoded.c_str(), -1, "utf-8",
								  charset.c_str(), 0,0,0);
				if (!utf8)
					return _("[Cannot decode this header line]");
				result += utf8;
				g_free (utf8);
			}

			i = j;
			// Maybe skip whitespace (see RFC 2047 section 6.2)
			while ((j < len) && ((copy[j] == ' ') || (copy[j] == '\t')))
				j++;
		}

		// ASCII character
		if (i < len)
			result += copy[i++];
	}

	return result;
}

/**
 *  Parse one encoded word in a message header.
 *  If this is successful true is returned and {\em charset} contains the
 *  charset, {\em encoding} the encoding, {\em text} the encoded text and
 *  pos the position of the first character after the encoded word. If this
 *  is not successful false is returned, {\em pos} is unchanged, the other
 *  values are undetermined.
 *
 *  @param  line      One (unfolded) message header line
 *  @param  charset   Here the character set of the encoded word is returned
 *                    (converted to lower case; only if the return value is
 *                    true)
 *  @param  encoding  Here the encoding of the encoded word is returned
 *                    (converted to lower case; only if the return value is
 *                    true)
 *  @param  text      Here the encoded text of the encoded word is returned
 *                    (only if the return value is true)
 *  @param  pos       Position of the encoded word in the line. When the return
 *                    value is true, {\em pos} is the position of the first
 *                    character after the encoded word
 *  @return           Boolean indicating success
 *
 *  @see  RFC 2047 sections 6.1 and 7
 */
gboolean 
Decoding::parse_encoded_word (const std::string &line, std::string &charset,
							  std::string &encoding, std::string &text,
							  std::string::size_type &pos)
{
	std::string::size_type i = pos, i1, i2, len = line.size();
	const std::string::size_type maxlen = 75; // see RFC 2047 section 2
	// For especials: see RFC 2047 section 2. '=' omitted because it's used by
	// the 'Q'-encoding
	const std::string especials = "()<>@,;:\"/[]?. ";

	// Test for "=?"
	if ((i+1 >= len) || (line[i] != '=') || (line[i+1] != '?'))
		return false;
	i += 2;

	// Search next "?"
	while ((i < len) && (i-pos < maxlen) && (!g_ascii_iscntrl (line[i]))
		   && (especials.find(line[i]) == std::string::npos))
		i++;
	if ((i >= len) || (i-pos >= maxlen) || (line[i] != '?'))
		return false;
	i1 = i++;

	// Store charset
	charset = ascii_strdown (line.substr (pos+2, i1-2-pos));

	// Search next "?"
	while ((i < len) && (i-pos < maxlen) && (!g_ascii_iscntrl (line[i]))
		   && (especials.find(line[i]) == std::string::npos))
		i++;
	if ((i >= len) || (i-pos >= maxlen) || (line[i] != '?'))
		return false;
	i2 = i++;

	// Store encoding
	encoding = ascii_strdown (line.substr (i1+1, i2-1-i1));

	// Search terminating "?="
	while ((i+1 < len) && (i+1-pos < maxlen) && (!g_ascii_iscntrl (line[i]))
		   && (especials.find(line[i]) == std::string::npos))
		i++;
	if ((i+1 >= len) || (i+1-pos >= maxlen) || (line[i] != '?')
		|| (line[i+1] != '='))
		return false;

	// Store text and update position
	text = line.substr (i2+1, i-1-i2);
	pos = i+2;

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
Decoding::get_quotedstring (std::string line, std::string &str,
							std::string::size_type &pos, gchar quoted,
							gboolean test_start, gboolean end_ok)
{
	std::string::size_type len = line.size ();
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
Decoding::get_mime_token (std::string line, std::string &str,
						  std::string::size_type &pos, gboolean lowercase)
{
	// Non alphanumeric characters allowed in tokens
	const static std::string token_ok = "!#$%&'*+-._`{|}~";

	std::string::size_type len = line.size();
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
	std::string::size_type pos = 0, len = todec.length();

	if (len%4 != 0)
		return std::string("");

	while (pos+3 < len) {
		if ((todec[pos]&0x80) || (todec[pos+1]&0x80) || (todec[pos+2]&0x80)
			|| (todec[pos+3]&0x80) || (index_64[(int)todec[pos]]<0)
			|| (index_64[(int)todec[pos+1]]<0))
			return std::string("");
		result += (gchar)((BASE64(pos) << 2) | (BASE64(pos+1) >> 4));
		if (todec[pos+2] == '=')
			if ((todec[pos+3]=='=') && (pos+4==len) && (!(BASE64(pos+1)&15)))
				return result;
			else
				return std::string("");
		if (index_64[(int)todec[pos+2]]<0)
			return std::string("");
		result += (gchar)(((BASE64(pos+1) & 0xf) << 4) | (BASE64(pos+2) >> 2));
		if (todec[pos+3] == '=')
			if ((pos+4 == len) && (!(BASE64(pos+2)&3)))
				return result;
			else
				return std::string("");
		if (index_64[(int)todec[pos+3]]<0)
			return std::string("");
		result += (gchar)(((BASE64(pos+2) & 0x3) << 6) | BASE64(pos+3));
		pos += 4;
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
	std::string::size_type pos = 0, len = todec.length();
	std::string result;
	gint decoded;

	while (pos < len)
	{
		switch (gchar c = todec.at(pos++))
		{
			case '=':
				pos += 2;
				if (pos > len)
					return result;
				if ((decoded  = g_ascii_xdigit_value(todec.at(pos-1))) < 0)
					return std::string("");
				if ((decoded += 16*g_ascii_xdigit_value(todec.at(pos-2))) < 0)
					return std::string("");
				result += decoded;
				break;
			case '_':
				result += ' ';
				break;
			default:
				result += c;
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
	std::string::size_type pos = 0, len = todec.length();
	std::string result;
	gint decoded;

	while (pos < len)
	{
		switch (gchar c=todec.at(pos++))
		{
			case '=':
				pos += 2;
				if (pos > len)
					return result;
				if ((decoded  = g_ascii_xdigit_value(todec.at(pos-1))) < 0)
					return std::string("");
				if ((decoded += 16*g_ascii_xdigit_value(todec.at(pos-2))) < 0)
					return std::string("");
				result += decoded;
				break;
			default:
				result += c;
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
								  std::string::size_type pos)
{
	std::string line;
	std::vector<std::string> result;

	while (pos < todec.size()) {
		// Handle soft breaks (see RFC 2045 6.7. (3),(5))
		line += todec[pos];
		std::string::size_type lpos = line.size();
		while ((lpos>0) && ((line[lpos-1]=='\t') || (line[lpos-1]==' ')))
			lpos--;
		if (lpos < line.size())
			line.erase (lpos, line.size());
		if ((line.size() > 0) && (line[line.size()-1] == '=')) {
			line.erase (line.size()-1);
			if (pos < todec.size()-1) {
				pos++;
				continue;
			}
		}
		// Decode line
		result.push_back (decode_quotedprintable (line));
		line = "";
		pos++;
	}
	return result;
}

/**
 *  This function converts an utf-8 encoded string to an imap modified
 *  utf-7 string. If the conversion is not successful an empty string is
 *  returned.
 *
 *  @param  str  Valid utf-8 encoded string to be converted
 *  @return      Converted string or empty string
 */
std::string 
Decoding::utf8_to_imaputf7 (const std::string str)
{
	gchar *buffer = utf8_to_imaputf7 (str.c_str(), -1);
	if (!buffer)
		return std::string ("");
	std::string result = std::string (buffer);
	g_free(buffer);
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
Decoding::utf8_to_imaputf7 (const gchar *str, gssize len)
{
	// Modified base64 characters (see RFC 2045, RFC 3501 5.1.3)
	const char *modbase64="ABCDEFGHIJKLMNOPQRSTUVWXYZ"
						  "abcdefghijklmnopqrstuvwxyz0123456789+,";

	// No String or nothing to do
	if ((str == NULL) || (len == 0))
		return NULL;

	gchar c = *str;;
	std::string result;
	gssize cnt_len = 0;
	gboolean printableascii = true;
	const gchar *start = str;

	while (((len<0) && (*str!='\0')) || (cnt_len<len) || (!printableascii))
	{
		if (cnt_len != len)
			c = *str;

		// End of non (printable) ASCII characters?
		if (((!printableascii) && (c>='\x20') && (c<='\x7e'))
			|| (((len<0) && (c=='\0')) || ((cnt_len>=len) && (len>0))))
		{
			result+='&';

			// Convert first to UTF-16
			gsize cnt = 0;
			gchar *utf16 = g_convert(start,str-start,"UTF-16BE","UTF-8",
									 NULL,&cnt,NULL);
			if (utf16 == NULL)
				return NULL;
			
			// Convert to modified BASE64
			gchar *pos = utf16;
			while (cnt > 0)
			{
				gchar b[5] = "\0\0\0\0";
				b[0] = modbase64[(pos[0]&0xfc)>>2];
				if (cnt == 1)
					b[1] = modbase64[(pos[0]&0x03)<<4];
				else
				{
					b[1] = modbase64[((pos[0]&0x03)<<4)|(pos[1]&0xf0)>>4];
					if (cnt == 2)
						b[2] = modbase64[(pos[1]&0x0f)<<2];
					else
					{
						b[2] = modbase64[((pos[1]&0x0f)<<2)|(pos[2]&0xc0)>>6];
						b[3] = modbase64[pos[2]&0x3f];
					}
				}
				result += b;
				if (cnt > 2)
					cnt -= 3;
				else
					cnt = 0;
				pos += 3;
			}

			g_free (utf16);
			result += "-";
			printableascii = true;
			continue;
		}

		cnt_len++;
		str++;
		  
		// (Printable) ASCII character?
		if ((printableascii) && (c>='\x20') && (c<='\x7e'))
		{
			result += c;
			if (c == '&')
				result += '-';
			continue;
		}

		// End of (printable) ASCII characters?
		if (printableascii)
		{
			printableascii = false;
			start = str-1;
			continue;
		}

		// Another non (printable) ASCII character!
	}

	return g_strdup (result.c_str ());
}

/**
 *  Convert the string {\em text} from the character set {\em charset} to
 *  utf-8. If no character set is given the string is assumed to be in the
 *  C runtime character set. If the string cannot be converted a error message
 *  is returned.
 *
 *  @param  text     String to be converted
 *  @param  charset  Character set of the string {\em text} or empty
 *  @return          Converted string or error message (as character array).
 *                   This string has to be freed with g_free().
 */
gchar * 
Decoding::charset_to_utf8 (std::string text, std::string charset)
{
	gchar *utf8 = (gchar *) text.c_str();
	if (!charset.empty())
		utf8 = g_convert (text.c_str(), -1, "utf-8", charset.c_str(), 0,0,0);
	else
		utf8 = g_locale_to_utf8 (text.c_str(), -1, 0, 0, 0);

	if (!utf8) {
		gchar *tmp = g_strdup_printf (_("[Cannot convert character sets "
										"(from \"%s\" to \"utf-8\")]"),
									  charset.empty() ? "C" : charset.c_str());
		utf8 = g_locale_to_utf8 (tmp, -1, 0, 0, 0);
		g_free (tmp);
	}

	return utf8;
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

	for (std::string::size_type j = 0; j < password.size(); j++)
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

	for (std::string::size_type i = 0; i+1 < password.size(); i += 2) {
		char c = 0;
		for (guint j = 0; j < 16; j++) {
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
