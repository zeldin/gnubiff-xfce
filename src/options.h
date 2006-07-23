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
// Author(s)     : Robert Sowada, Nicolas Rougier
// Short         : Container for storing options
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif
#include <glade/glade.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <map>
#include <set>
#include <string>
#include "option.h"

enum OptionsGUI { OPTSGUI_GET = 1,
				  OPTSGUI_SET = 2,
				  OPTSGUI_SENSITIVE = 4,
				  OPTSGUI_SHOW = 8,
				  OPTSGUI_UPDATE = 14 // SET | SENSITIVE | SHOW
};

class Options {
public:
	Options ();
	virtual ~Options ();

	gboolean add_group (Option_Group *group);
	gboolean add_option (Option *option);
	gboolean add_option (Options &options);
	gboolean remove_options (guint flags);
	Option *find_option (const std::string &name,OptionType type=OPTTYPE_NONE);

	gboolean reset (const std::string &name, gboolean respect_change = true);
	gboolean value (const std::string &name, gboolean value,
					gboolean respect_change = true);
	gboolean value (const std::string &name, guint value,
					gboolean respect_change = true);
	gboolean value (const std::string &name, std::string value,
					gboolean respect_change = true);
	gboolean value_bool (const std::string &name,
						 gboolean respect_update = true);
	std::string value_string (const std::string &name,
							  gboolean respect_update = true);
	const gchar * value_gchar (const std::string &name,
							   gboolean respect_update = true);
	guint value_uint (const std::string &name, gboolean respect_update = true);
	gboolean set_values (const std::string &name,
						 const std::set<std::string> &values,
						 gboolean empty = true,gboolean respect_change = true);
	gboolean get_values (const std::string &name, std::set<std::string> &var,
						 gboolean empty = true,gboolean respect_update = true);
	gboolean get_values (const std::string &name,
						 std::vector<std::string> &var,
						 gboolean empty = true,gboolean respect_update = true);
	const std::string value_to_string (const std::string &name, guint val);
	std::string to_string (const std::string &name,
						   gboolean respect_update = true);
	gboolean from_string (const std::string &name, const std::string value,
						  gboolean respect_change = true);
	guint string_to_value (const std::string &name, const std::string &str);

	void to_strings (guint groups, std::map<std::string,std::string> &map,
					 gboolean nosave = true, gboolean empty = true);
	gboolean from_strings (guint groups,
						   std::map<std::string,std::string> &map);

	void update_gui (GladeXML *xml, const std::string filename);
	void update_gui (OptionsGUI whattodo, guint groups, GladeXML *xml,
					 const std::string filename);
	void update_gui (OptionsGUI whattodo, Option *option, GladeXML *xml,
					 const std::string filename);

	std::string group_help (guint group);
	std::string group_name (guint group);
	/// Access function to Options::groups_
	std::map<guint, Option_Group *> *groups (void) {return &groups_;}
	/// Access function to Options::options_
	std::map<std::string, Option *> *options (void) {return &options_;}
protected:
	/// Iterator
	typedef std::map<std::string, Option *>::iterator iterator;
	/// Iterator for groups
	typedef std::map<guint, Option_Group *>::iterator iterator_group;
	/// Iterator for stored widgets/options map
	typedef std::map<std::string, std::set<std::string> >::iterator iterator_widgets;
	/// Stored options
	std::map<std::string, Option *> options_;
	/// Stored groups
	std::map<guint, Option_Group *> groups_;

	GtkWidget *get_widget (const gchar *name, GladeXML *xml,
						   const gchar *filename);
	void store_widgets (const std::string name,const std::set<std::string> &gs,
						std::map<std::string, std::set<std::string> > &map);

	/**
	 *  This function is called when an option is changed that has the
	 *  OPTFLG_CHANGE flag set.
	 *
	 *  @param option Pointer to the option that is changed.
	 */
	virtual void option_changed (Option *option) {};
	/**
	 *  This function is called when an option is to be read that needs
	 *  updating before. These options have to be marked by the OPTFLG_UPDATE
	 *  flag.
	 *
	 *  @param option Pointer to the option that is to be updated.
	 */
	virtual void option_update (Option *option) {};
private:
	/**
	 *  Map of widgets and a set of boolean options. If all boolean
	 *  options are true (false if the option's name is preceded by a
	 *  '!') then the widget is sensitive, otherwise it isn't set to
	 *  be sensitive.
	 */
	std::map<std::string, std::set<std::string> > widgets_sensitive_;
	/**
	 *  Map of widgets and a set of boolean options. If all boolean
	 *  options are true (false if the option's name is preceded by a
	 *  '!') then the widget is shown, otherwise it's hidden.
	 */
	std::map<std::string, std::set<std::string> > widgets_show_;
};

#endif
