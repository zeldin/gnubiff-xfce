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
// Author(s)     : Nicolas Rougier
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
	// ================================================================================
	//  general
	// ================================================================================
	std::string		passtable_;					// encryption table

protected:
	// ================================================================================
	//  internal
	// ================================================================================
	std::vector<class Mailbox *>	mailbox_;		// mailboxes
	GMutex *						mutex_;			// access mutex
	class Authentication			*ui_auth_;		// ui to get username & password
	GMutex							*ui_auth_mutex_;// Lock to avoid conflicts
	class Preferences	*			preferences_;	// preferences ui
	class Popup *					popup_;			// popup ui
	class Applet *					applet_;		// applet ui
	/// Buffer for temporary saving values when loading the config file
	std::map<std::string,std::string> buffer_load_;

public:
	// ================================================================================
	//  base
	// ================================================================================
	Biff (guint ui_mode = GTK_MODE,
		  std::string filename = "");
	~Biff (void);

	// ================================================================================
	//  access
	// ================================================================================
	guint size (void);
	gboolean find_mail (std::string mailid, Header &mail);
	class Mailbox * mailbox (guint index);
	class Mailbox * get (guint uin);
	class Preferences *preferences (void)		{return preferences_;}
	class Applet *applet (void)					{return applet_;}
	class Popup *popup (void)					{return popup_;}
	

	// ================================================================================
	//  main
	// ================================================================================
	void add (Mailbox *mailbox);					// add a new mailbox
	Mailbox *replace (Mailbox *from, Mailbox *to);	// replace a mailbox (from) with another (to)
	void remove (Mailbox *mailbox);					// remove a mailbox
	gboolean password (Mailbox *mailbox);		// try to find a password for this mailbox
	void option_changed (Option *option);
	void option_update (Option *option);

	// ================================================================================
	//  i/o
	// ================================================================================
	gboolean load (void);
protected:
	std::vector<const gchar *> save_blocks;
	std::stringstream save_file;
	void save_newblock (const gchar *name);
	void save_endblock (void);
	void upgrade_options (void);
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
