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

/**
 * Constant definitions
 **/
const guint	GTK_MODE		=	0;
const guint	GNOME_MODE		=	1;
const guint	MANUAL_CHECK	=	0;
const guint	AUTOMATIC_CHECK	=	1;


#define BIFF(x)		((Biff *)(x))


class Biff {

public:
	// ================================================================================
	//  general
	// ================================================================================
	std::string		passtable_;					// encryption table
	guint			ui_mode_;					// GTK or GNOME mode
	guint			check_mode_;				// gnubiff check mode
	gboolean		use_max_mail_;				// whether to restrict maximum collected email
	guint			max_mail_;					// maximum collected email
	gboolean		use_newmail_command_;		// whether to run a command on new mail
	std::string		newmail_command_;			// command to run when new mail
	gboolean		use_double_command_;		// whether to run a command when double clicked
	std::string		double_command_;			// command to run when double clicked
	/// Buffer for temporary saving values when loading the config file
	std::map<std::string,std::string> buffer_load_;

	// ================================================================================
	//  applet
	// ================================================================================
	gboolean		applet_use_geometry_;		// whether applet use geometry
	std::string		applet_geometry_;			// applet geometry
	gboolean		applet_use_decoration_;		// whether applet uses decoration
	gboolean        applet_be_sticky_;          // whether applet should be sticky
	gboolean        applet_keep_above_;         // whether applet window should be kept always on top
	gboolean		applet_pager_;				// whether applet should appear in a pager
	std::string		applet_font_;				// applet font
	gboolean		use_newmail_text_;			// wheter text is displayed when new mail
	std::string		newmail_text_;				// applet text when new mail
	gboolean		use_newmail_image_;			// wheter image is displayed when new mail
	std::string		newmail_image_;				// applet image filename for new mail
	gboolean		use_nomail_text_;			// wheter text is displayed when no mail
	std::string		nomail_text_;				// applet text when no mail
	gboolean		use_nomail_image_;			// wheter image is displayed when no mail
	std::string		nomail_image_;				// applet image filename for no mail

	// ================================================================================
	//  popup
	// ================================================================================
	gboolean		use_popup_;					// whether to use popup
	guint			popup_delay_;				// amount of time to display popup
	gboolean		popup_use_geometry_;		// whether popup use geometry
	std::string		popup_geometry_;			// popup geometry
	gboolean		popup_use_decoration_;		// whether popup uses decoration
	gboolean        popup_be_sticky_;           // whether popup should be sticky
	gboolean        popup_keep_above_;          // whether popup window should be kept always on top
	gboolean		popup_pager_;				// whether popup window should appear in a pager
	std::string		popup_font_;				// popup font
	gboolean		popup_use_size_;			// whether popup reestrict number of displayed header
	guint			popup_size_;				// maximum header to display
	gboolean		popup_use_format_;			// whether popup use format
	std::string		popup_format_;				// popup format
	guint			sender_size_;				// sender field size
	guint			subject_size_;				// subject field size
	guint			date_size_;					// date field size


protected:
	// ================================================================================
	//  internal
	// ================================================================================
	std::string						filename_;		// configuration file
	std::vector<class Mailbox *>	mailbox_;		// mailboxes
	GMutex *						mutex_;			// access mutex
	class Authentication			*ui_auth_;		// ui to get username & password
	GMutex							*ui_auth_mutex_;// Lock to avoid conflicts
	class Preferences	*			preferences_;	// preferences ui
	class Popup *					popup_;			// popup ui
	class Applet *					applet_;		// applet ui


public:
	// ================================================================================
	//  base
	// ================================================================================
	Biff (gint ui_mode=GTK_MODE,
		  std::string filename = "");
	~Biff (void);

	// ================================================================================
	//  access
	// ================================================================================
	guint size (void);
	gboolean find_mail (std::string mailid, struct header_ &mail);
	void popup_format (std::string format);
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

	// ================================================================================
	//  i/o
	// ================================================================================
	gboolean load (void);
protected:
	std::vector<const gchar *> save_blocks;
	std::stringstream save_file;
	void save_newblock(const gchar *);
	void save_endblock(void);
public:
	void save_para(const gchar *name, const std::string value);
	void save_para(const gchar *name, const std::set<std::string> &value);
	void save_para(const gchar *name, guint value);
	void save_para(const gchar *name, gboolean value);
	void load_para(const gchar *name, guint &var);
	void load_para(const gchar *name, std::string &var);
	void load_para(const gchar *name, gboolean &var);
	void load_para(const gchar *name, std::set<std::string> &var);
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
