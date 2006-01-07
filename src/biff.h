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
// Short         : 
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#ifndef __BIFF_H__
#define __BIFF_H__

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif
#include <set>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <glib.h>
#include "decoding.h"
#include "gnubiff_options.h"
#include "header.h"


#define BIFF(x)		((Biff *)(x))


class Biff : public Gnubiff_Options {

public:
	// ========================================================================
	//  general
	// ========================================================================


protected:
	// ========================================================================
	//  internal
	// ========================================================================
	// All mailboxes that are being monitored
	std::vector<class Mailbox *>	mailbox_;
	/**
	 *  Access mutex. This mutex has to be locked for working with the
	 *  mailboxes.
	 */
	GMutex *						mutex_;
	/// Mutex for obtaining passwords
	GMutex							*auth_mutex_;
	/// Applet user interface
	class Applet *					applet_;
	/// Buffer for temporary saving values when loading the config file
	std::map<std::string,std::string> buffer_load_;

public:
	// ========================================================================
	//  base
	// ========================================================================
	Biff (guint ui_mode = MODE_GTK, std::string filename = "");
	~Biff (void);

	// ========================================================================
	//  access
	// ========================================================================
	gboolean find_message (std::string mailid, Header &mail);
	class Mailbox * mailbox (guint index);
	class Mailbox * get (guint uin);
	class Applet *applet (void)					{return applet_;}

	// ========================================================================
	//  main -- mailbox handling
	// ========================================================================
	void add_mailbox (Mailbox *mailbox);
	guint get_number_of_mailboxes (void);
	std::vector<Header *> get_message_headers (gboolean use_max_num = false,
											   guint max_num = 0);
	gboolean get_number_of_unread_messages (guint &num);
	gboolean get_password_for_mailbox (Mailbox *mailbox);
	void mark_messages_as_read (void);
	void messages_displayed (void);
	void remove_mailbox (Mailbox *mailbox);
	Mailbox *replace_mailbox (Mailbox *from, Mailbox *to);
	void start_monitoring (guint delay = 0);
	void stop_monitoring (void);

	// ========================================================================
	//  options
	// ========================================================================
	void option_changed (Option *option);
	void option_update (Option *option);
	void upgrade_options (void);

	// ========================================================================
	//  i/o
	// ========================================================================
	gboolean load (void);
protected:
	std::vector<const gchar *> save_blocks;
	std::stringstream save_file;
	void save_newblock (const gchar *name);
	void save_endblock (void);
public:
	void save_parameters (std::map<std::string,std::string> &map,
						  std::string block = std::string(""));
	gboolean save (void);
	void xml_start_element (GMarkupParseContext *context,
							const gchar *element_name,
							const gchar **attribute_names,
							const gchar **attribute_values,
							GError **error);
	void xml_end_element (GMarkupParseContext *context,
						  const gchar *element_name, GError **error);
	void xml_error (GMarkupParseContext *context,
					GError *error);
};

#endif
