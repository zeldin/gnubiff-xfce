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

#ifndef __HEADER_H__
#define __HEADER_H__

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

/**
 *  All the information about a specific mail needed by gnubiff. Headers are
 *  referred to by the mailid.
 */
class Header {
public:
	void add_to_body (const std::string text);
	static gboolean sort_headers (std::vector<Header *> &ptr_headers,
								  std::string sort_order);

protected:
	/// First lines of the mail' body
	std::string		body_;
	/// Characterset of the mail's body
	std::string		charset_;
	/// Date of the mail
	std::string		date_;
	/// Key for faster sorting by date
	std::string date_collate_key_;
	/// Mailbox identifier
	guint			mailbox_uin_;
	/**
	 *  This is a (hopefully) unique identifier for the mail. If supported by
	 *  the protocol this will be the unique id of the mail that is provided
	 *  by the server (this is the case for POP3 and IMAP4). Otherwise
	 *  gnubiff creates an own identifier.
	 *
	 *  Remark: This identifier must not contain whitespace characters!
	 *
	 *  @see The mail identifier is calculated by the method Header::mailid().
	 */
	std::string mailid_;
	/// Position in the mailbox
	guint position_;
	/// Sender of the mail
	std::string	sender_;
	/// Key for faster sorting by sender
	std::string sender_collate_key_;
	/// Subject of the mail
	std::string	subject_;
	/// Key for faster sorting by subject
	std::string subject_collate_key_;

public:
	bool operator == (const Header &other) const;

	/* Binary function typedef. Needed to define the structures for comparing
	 * headers when sorting. */
	typedef std::binary_function<Header *, Header *, bool> bin_fun_;

	/// Comparing the position of two headers
	struct compare_position : public bin_fun_
	{
		bool operator()(const Header *x, const Header *y) const
		{
			return x->position() > y->position();
		}
	};

	/// Comparing the position of two headers (reversed)
	struct compare_position_rev : public bin_fun_
	{
		bool operator()(const Header *x, const Header *y) const
		{
			return x->position() < y->position();
		}
	};

	/// Comparing the mailbox identifier of two headers
	struct compare_mailbox_uin : public bin_fun_
	{
		bool operator()(const Header *x, const Header *y) const
		{
			return x->mailbox_uin() > y->mailbox_uin();
		}
	};

	/// Comparing the mailbox identifier of two headers (reversed)
	struct compare_mailbox_uin_rev : public bin_fun_
	{
		bool operator()(const Header *x, const Header *y) const
		{
			return x->mailbox_uin() < y->mailbox_uin();
		}
	};

	/// Comparing the sender of two headers
	struct compare_sender : public bin_fun_
	{
		bool operator()(const Header *x, const Header *y) const
		{
			return (x->sender_sort() > y->sender_sort());
		}
	};

	/// Comparing the sender of two headers (reversed)
	struct compare_sender_rev : public bin_fun_
	{
		bool operator()(const Header *x, const Header *y) const
		{
			return (x->sender_sort() < y->sender_sort());
		}
	};

	/// Comparing the subject of two headers
	struct compare_subject : public bin_fun_
	{
		bool operator()(const Header *x, const Header *y) const
		{
			return (x->subject_sort() > y->subject_sort());
		}
	};

	/// Comparing the subject of two headers (reversed)
	struct compare_subject_rev : public bin_fun_
	{
		bool operator()(const Header *x, const Header *y) const
		{
			return (x->subject_sort() < y->subject_sort());
		}
	};

	// FIXME: Date comparisions should not be string based!
	/// Comparing the date of two headers
	struct compare_date : public bin_fun_
	{
		bool operator()(const Header *x, const Header *y) const
		{
			return (x->date_sort() > y->date_sort());
		}
	};

	// FIXME: Date comparisions should not be string based!
	/// Comparing the date of two headers (reversed)
	struct compare_date_rev : public bin_fun_
	{
		bool operator()(const Header *x, const Header *y) const
		{
			return (x->date_sort() < y->date_sort());
		}
	};

	/// Access function to Header::body_
	std::string body (void) const {return body_;}
	/// Access function to Header::body_
	std::string date (void) const {return date_;}
	void date (const std::string date);
	/// Access function to Header::date_collate_key_
	std::string date_sort (void) const {return date_collate_key_;}
	/// Access function to Header::body_
	void body (const std::string body) {body_ = body;}
	/// Access function to Header::charset_
	std::string charset (void) const {return charset_;}
	/// Access function to Header::charset_
	void charset (const std::string charset) {charset_ = charset;}
	/// Access function to Header::mailbox_uin_
	guint mailbox_uin (void) const {return mailbox_uin_;}
	/// Access function to Header::mailbox_uin_
	void mailbox_uin (guint pos) {mailbox_uin_ = pos;}
	/// Access function to Header::mailid_
	std::string mailid (void) const {return mailid_;}
	void mailid (const std::string uid);
	/// Access function to Header::position_
	guint position (void) const {return position_;}
	/// Access function to Header::position_
	void position (guint pos) {position_ = pos;}
	/// Access function to Header::sender_
	std::string sender (void) const {return sender_;}
	void sender (const std::string sender);
	/// Access function to Header::sender_collate_key_
	std::string sender_sort (void) const {return sender_collate_key_;}
	/// Access function to Header::subject_
	std::string subject (void) const {return subject_;}
	void subject (const std::string subject);
	/// Access function to Header::subject_collate_key_
	std::string subject_sort (void) const {return subject_collate_key_;}
};

#endif
