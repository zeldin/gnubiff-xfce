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
// Short         : All information about a specific mail needed by gnubiff
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#include <algorithm>
#include <glib.h>
#include <string>
#include <sstream>
#include <vector>
#include "header.h"
#include "nls.h"

/**
 *  Test whether two headers belong to the same mail.
 *
 *  @param   other  Reference to another header
 *  @returns        True if the two mails are equal, false otherwise
 */
bool 
Header::operator == (const Header &other) const
{
	return (mailid_ == other.mailid_);
}

/**
 *  Substitute mail body by error message (if such a message is present).
 *  If no error message is present nothing is done.
 */
void 
Header::error_to_body (void)
{
	if (error_.size() == 0)
		return;

	// Substitute body by error message and update charset (error messages are
	// in the charset given by the locale)
	body (error_);
	charset ("");
}

/**
 *  Add some text to the mail's body.
 *
 *  @param text Text to add
 */
void 
Header::add_to_body (const std::string text)
{
	body_ += text;
}

/**
 *  Sort a vector of (pointers to) headers. The properties of the headers by
 *  which this vector shall be sorted is given by a space separated list of
 *  strings. Sorted can be by position in the mailbox ("position"), mailbox
 *  identifier ("mailbox"), sender ("sender"), subject ("subject") and date
 *  ("date"). By preceding a "!" the sort order can be reversed.
 *
 *  @param  ptr_headers Reference to the vector of headers to be sorted.
 *  @param  sort_order  Space separated list giving header properties
 *  @return             True if the last property is "mailbox", false otherwise
 */
gboolean 
Header::sort_headers (std::vector<Header *> &ptr_headers,
					  std::string sort_order)
{
	std::stringstream ss (sort_order);
	std::string sortby;
	std::vector<Header *>::iterator itb = ptr_headers.begin ();
	std::vector<Header *>::iterator ite = ptr_headers.end ();
	gboolean mb_end = false;
	while (ss >> sortby) {
		mb_end = false;

		// Reversed order?
		gboolean reversed = (sortby[0] == '!');
		if (reversed)
			sortby = sortby.substr (1, sortby.size()-1);

		if (sortby == "position") {
			if (reversed)
				std::stable_sort(itb, ite, Header::compare_position());
			else
				std::stable_sort(itb, ite, Header::compare_position_rev());
			continue;
		}

		if (sortby == "mailbox") {
			if (reversed)
				std::stable_sort(itb, ite, Header::compare_mailbox_uin());
			else
				std::stable_sort(itb, ite, Header::compare_mailbox_uin_rev());
			mb_end = true;
			continue;
		}

		if (sortby == "sender") {
			if (reversed)
				std::stable_sort(itb, ite, Header::compare_sender());
			else
				std::stable_sort(itb, ite, Header::compare_sender_rev());
			continue;
		}

		if (sortby == "subject") {
			if (reversed)
				std::stable_sort(itb, ite, Header::compare_subject());
			else
				std::stable_sort(itb, ite, Header::compare_subject_rev());
			continue;
		}

		if (sortby == "date") {
			if (reversed)
				std::stable_sort(itb, ite, Header::compare_date());
			else
				std::stable_sort(itb, ite, Header::compare_date_rev());
			continue;
		}

		// Otherwise: Ignore it and print error message
		g_warning (_("Can't sort mails by \"%s\""), sortby.c_str());
	}

	return mb_end;
}

/**
 *  Set the date of the mail. This function also computes a key for faster
 *  sorting mails by date.
 *
 *  @param date Date of the mail
 */
void 
Header::date (const std::string date)
{
	date_ = date;

	// For date and time specification see RFC 2822 3.3
	std::stringstream ss (date);
	std::string temp;
	gint day = 0, month = 0, year = 0, hour = 0, minute = 0, second = 0;
	gint zone_hour = 0, zone_minute = 0, zone_sign = 1;

	// day-of-week & day
	if (date[3] == ',')
		ss >> temp;
	ss >> day;
	day = std::max<gint>(day, 1);
	// month
	static const std::string months = "JanFebMarAprMayJunJulAugSepOctNovDec";
	ss >> temp;
	month = months.find (temp);
	if (((std::string::size_type)month == std::string::npos) || (month%3 != 0))
		month = 0;
	month = month / 3 + 1;
	// year
	ss >> year;
	year = std::max<gint>(year, 1900);
	day=std::min<gint>(day, g_date_get_days_in_month((GDateMonth)month, year));
	// time-of-day
	ss >> temp;
	if ((temp.size() == 5) || (temp.size() == 8)) {
		hour   = std::min<gint>(10*(temp[0]-'0') + (temp[1]-'0'), 23);
		minute = std::min<gint>(10*(temp[3]-'0') + (temp[4]-'0'), 59);
		if (temp.size() == 8)
			second = std::min<gint>(10*(temp[6]-'0') + (temp[7]-'0'), 60);
		hour = std::max (hour, 0);
		minute = std::max (minute, 0);
		second = std::max (second, 0);
	}
	// zone
	ss >> temp;
	if (temp.size() == 5) {
		zone_hour   = std::min<gint>(10*(temp[1]-'0') + (temp[2]-'0'), 99);
		zone_minute = std::min<gint>(10*(temp[3]-'0') + (temp[4]-'0'), 59);
		if (temp[0] == '-')
			zone_sign = -1;
		zone_hour = std::max (zone_hour, 0);
		zone_minute = std::max (zone_minute, 0);
	}

	// Calculate date
	// minute
	minute -= zone_sign*zone_minute;
	hour += minute/60;
	minute = minute%60;
	if (minute < 0) {
		hour--;
		minute += 60;
	}
	// hour
	hour -= zone_sign*zone_hour;
	day += hour/24;
	hour = hour%24;
	if (hour < 0) {
		day--;
		hour += 24;
	}
	// day, month, year
	if (day <= 0) {
		if (month-- == 0) {
			year--;
			month = 12;
		}
		day += g_date_get_days_in_month ((GDateMonth)month, year);
	}
	else if (day > g_date_get_days_in_month ((GDateMonth)month, year)) {
		day -= g_date_get_days_in_month ((GDateMonth)month, year);
		if (month++ == 13) {
			year++;
			month = 1;
		}
	}

	// Create date sort key
	gchar *date_str = g_strdup_printf ("%04d%02d%02d%02d%02d%02d", year, month,
									   day, hour, minute, second);

	if (date_str) {
		gchar *tmp = g_utf8_collate_key (date_str, -1);
		if (tmp) {
			date_collate_key_ = tmp;
			g_free (tmp);
		}
		g_free (date_str);
	}
}

/**
 *  Setting the gnubiff mail identifier for this mail header. If a unique
 *  identifier {\em uid} is provided it is taken. Otherwise (i.e.
 *  {\em uid} is an empty string) it is created by concatenating hash
 *  values of the sender, subject and date.
 *
 *  @param uid Unique identifier for the mail as provided by the protocol
 *             (POP3 and IMAP4) or an empty string.
 */
void 
Header::mailid (const std::string uid = std::string(""))
{
	if (uid.size () > 0)
		mailid_ = uid;
	else {
		std::stringstream ss;
		ss << g_str_hash (sender_.c_str());
		ss << g_str_hash (subject_.c_str());
		ss << g_str_hash (date_.c_str());
		mailid_ = ss.str ();
	}
}

/**
 *  Set the sender of the mail. This function also computes a key for faster
 *  sorting mails by sender.
 *
 *  @param sender Sender of the mail
 */
void 
Header::sender (const std::string sender)
{
	sender_ = sender;
	gchar *tmp = g_utf8_collate_key (sender.c_str (), -1);
	if (tmp) {
		sender_collate_key_ = tmp;
		g_free (tmp);
	}
}

/**
 *  Set the subject of the mail. This function also computes a key for faster
 *  sorting mails by subject.
 *
 *  @param subject Subject of the mail
 */
void 
Header::subject (const std::string subject)
{
	subject_ = subject;
	gchar *tmp = g_utf8_collate_key (subject.c_str (), -1);
	if (tmp) {
		subject_collate_key_ = tmp;
		g_free (tmp);
	}
}
