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
// Author(s)     : Robert Sowada, Nicolas Rougier
// Short         : Container for storing options
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#include <sstream>
#include "nls.h"
#include "options.h"

Options::Options ()
{
}

Options::~Options ()
{
	iterator opt = options_.begin ();
	while (opt != options_.end ())
		delete (opt++)->second;

	iterator_group grp = groups_.begin ();
	while (grp != groups_.end ())
		delete (grp++)->second;
}

gboolean 
Options::add_group (Option_Group *group)
{
	if ((group == NULL) || (groups_.find (group->id()) != groups_.end()))
		return false;
	groups_[group->id()] = group;
	return true;
}

gboolean 
Options::add_option (Option *option)
{
	if ((option == NULL) || (options_.find (option->name()) != options_.end()))
		return false;
	options_[option->name()] = option;
	return true;
}

/**
 *  Add all options (and their values) from {\em options} to the set of
 *  options. If a option already exists it gets the value from the same option
 *  in {\em options}. This function also adds new groups from {\em options}
 *  to the set of groups but does not change existing groups.
 *
 *  @param  options Options to be added.
 *  @return         True if all options could be added successfully.
 */
gboolean 
Options::add_option (Options &options)
{
	gboolean ok = true;

	// Options
	std::map<std::string, Option *> *opts = options.options ();
	iterator opt =  opts->begin ();
	while (opt != opts->end ()) {
		// When we already have the option copy the value, otherwise create it
		Option *option = find_option (opt->second->name(), OPTTYPE_NONE);
		if (option)
			ok = option->from_string (opt->second->to_string()) && ok;
		else
			ok = add_option (opt->second->copy()) && ok;
		opt++;
	}

	// Groups
	std::map<guint, Option_Group *> *grps = options.groups ();
	std::map<guint, Option_Group *>::iterator grp =  grps->begin ();
	while (grp != grps->end ()) {
		Option_Group *group = (grp++)->second;
		// If we don't have the group: Create it
		if (groups_.find (group->id()) == groups_.end())
			ok = add_group (new Option_Group (group->name(), group->id(),
											  group->help()));
	}

	return ok;
}

/**
 *  Reset the option to the default value. This function handles the
 *  OPTFLG_CHANGE flag, so use this instead of calling Option::reset()
 *  directly.
 *
 *  @param  name           Name of the option
 *  @param  respect_change Respect the OPTFLG_CHANGE flag (default is true)
 *  @return                Success indicator
 */
gboolean 
Options::reset (const std::string &name, gboolean respect_change)
{
	Option *option = find_option (name);
	if (option) {
		option->reset ();
		if (option->flags() & OPTFLG_CHANGE)
			option_changed (option);
	}
	return (option != NULL);
}

gboolean 
Options::value (const std::string &name, gboolean value,
				gboolean respect_change)
{
	Option_Bool *option = (Option_Bool *) find_option (name, OPTTYPE_BOOL);
	if (option) {
		option->value (value);
		if ((option->flags() & OPTFLG_CHANGE) && respect_change)
			option_changed (option);
	}
	return (option != NULL);
}

gboolean 
Options::value (const std::string &name, std::string value,
				gboolean respect_change)
{
	Option_String *option=(Option_String *) find_option (name, OPTTYPE_STRING);
	if (option) {
		option->value (value);
		if ((option->flags() & OPTFLG_CHANGE) && respect_change)
			option_changed (option);
	}
	return (option != NULL);
}

gboolean 
Options::value (const std::string &name, guint value, gboolean respect_change)
{
	Option_UInt *option = (Option_UInt *) find_option (name, OPTTYPE_UINT);
	if (option) {
		option->value (value);
		if ((option->flags() & OPTFLG_CHANGE) && respect_change)
			option_changed (option);
	}
	return (option != NULL);
}

gboolean 
Options::value_bool (const std::string &name, gboolean respect_update)
{
	Option_Bool *option = (Option_Bool *) find_option (name, OPTTYPE_BOOL);
	if (!option)
		return false;
	if ((option->flags() & OPTFLG_UPDATE) && respect_update)
		option_update (option);
	return option->value();
}

std::string 
Options::value_string (const std::string &name, gboolean respect_update)
{
	Option_String *option=(Option_String *) find_option (name, OPTTYPE_STRING);
	if (!option)
		return std::string("");
	if ((option->flags() & OPTFLG_UPDATE) && respect_update)
		option_update (option);
	return option->value();
}

const gchar *
Options::value_gchar (const std::string &name, gboolean respect_update)
{
	return value_string(name, respect_update).c_str();
}

guint 
Options::value_uint (const std::string &name, gboolean respect_update)
{
	Option_UInt *option = (Option_UInt *) find_option (name, OPTTYPE_UINT);
	if (!option)
		return 0;
	if ((option->flags() & OPTFLG_UPDATE) && respect_update)
		option_update (option);
	return option->value();
}

gboolean 
Options::set_values (const std::string &name,
					 const std::set<std::string> &values, gboolean empty,
					 gboolean respect_change)
{
	Option_String *option=(Option_String *) find_option (name, OPTTYPE_STRING);
	if (option) {
		option->set_values (values, empty);
		if ((option->flags() & OPTFLG_CHANGE) && respect_change)
			option_changed (option);
	}
	return (option != NULL);
}

gboolean 
Options::get_values (const std::string &name, std::set<std::string> &var,
					 gboolean empty, gboolean respect_update)
{
	Option_String *option=(Option_String *) find_option (name, OPTTYPE_STRING);
	if (!option)
		return false;
	if ((option->flags() & OPTFLG_UPDATE) && respect_update)
		option_update (option);
	option->get_values (var, empty);
	return true;
}

const std::string 
Options::value_to_string (const std::string &name, guint val)
{
	Option_UInt *option = (Option_UInt *) find_option (name, OPTTYPE_UINT);
	if (!option)
		return std::string("");
	return option->value_to_string (val);
}

std::string 
Options::to_string (const std::string &name, gboolean respect_update)
{
	Option *option = find_option (name);
	if (!option)
		return std::string ("");
	if ((option->flags() & OPTFLG_UPDATE) && respect_update)
		option_update (option);
	return option->to_string ();
}

gboolean 
Options::from_string (const std::string &name, const std::string value,
					  gboolean respect_change)
{
	Option *option = find_option (name);
	if (!option)
		return false;
	option->from_string (value);
	if ((option->flags() & OPTFLG_CHANGE) && respect_change)
		option_changed (option);
	return true;
}


guint 
Options::string_to_value (const std::string &name, const std::string &str)
{
	Option_UInt *option = (Option_UInt *) find_option (name, OPTTYPE_UINT);
	if (!option)
		return 0;
	return option->string_to_value (str);
}

void 
Options::to_strings (guint groups, std::map<std::string,std::string> &map,
					 gboolean nosave, gboolean empty)
{
	if (empty)
		map.clear ();

	iterator opt = options_.begin ();
	while (opt != options_.end ()) {
		Option *option = opt->second;
		if (option && (groups & option->group ())
			&& (!nosave || !(option->flags() & OPTFLG_NOSAVE))) {
			if (option->flags() & OPTFLG_UPDATE)
				option_update (option);
			map[option->name ()] = option->to_string ();
		}
		opt++;
	}
}

gboolean 
Options::from_strings (guint groups, std::map<std::string,std::string> &map)
{
	gboolean ok = true;
	std::map<std::string,std::string>::iterator it=map.begin ();

	while (it != map.end ()) {
		iterator opt = options_.find (it->first);
		if (opt == options_.end()) {
			ok = false;
			g_warning(_("Unknown option \"%s\""), it->first.c_str());
		}
		else {
			Option *option = opt->second;
			if (option && (groups & option->group ())) {
				if (!(option->from_string (it->second))) {
					ok = false;
					g_warning(_("Cannot set option \"%s\" to \"%s\""),
							  it->first.c_str(), it->second.c_str());
				}
				else if (option->flags() & OPTFLG_CHANGE)
					option_changed (option);
			}
		}
		it++;
	}
	return ok;
}

/**
 *  Update GUI widgets for a group of options.
 *
 *  @param  whattodo Actions to be done
 *  @param  groups   Groups from which all options are taken
 *  @param  xml      GladeXML information of the GUI
 *  @param  filename Filename of the glade file for the GUI
 */
void 
Options::update_gui (OptionsGUI whattodo, guint groups, GladeXML *xml,
					 const std::string filename)
{
	iterator opt = options_.begin ();
	while (opt != options_.end ()) {
		Option *option = opt->second;
		opt++;
		if (!option || !(groups & option->group ()))
			continue;
		update_gui (whattodo, option, xml, filename);
	}
}

/**
 *  Update GUI widgets for the option {\em option}. The following actions can
 *  be done (in the given order):
 *  \begin{itemize}
 *     \item Get the value from the widget and set {\em option} (OPTSGUI_GET)
 *     \item Set the widget to the value of {\em option} (OPTSGUI_SET)
 *     \item Update the widgets that are sensitive to {\em option}
 *           (OPTSGUI_SENSITIVE)
 *     \item Show and hide widgets (OPTSGUI_SHOW)
 *  \end{itemize}
 *  OPTSGUI_UPDATE is short for OPTSGUI_SET, OPTSGUI_SENSITIVE and
 *  OPTSGUI_SHOW.
 *
 *  @param  whattodo Actions to be done
 *  @param  option   Option for which the update is done
 *  @param  xml      GladeXML information of the GUI
 *  @param  filename Filename of the glade file for the GUI
 */
void 
Options::update_gui (OptionsGUI whattodo, Option *option, GladeXML *xml,
					 const std::string filename)
{
	if (!option)
		return;
	const gchar *file = filename.c_str ();

	// Get widgets
	std::stringstream ss (option->gui_name());
	std::string gui_name;
	std::vector<GtkWidget *> widgets;
	while (ss >> gui_name)
		widgets.push_back (get_widget (gui_name.c_str(), xml, file));

	if (whattodo & OPTSGUI_GET)
		option->get_gui (widgets);

	if (whattodo & OPTSGUI_SET)
		option->set_gui (widgets);

	if (whattodo & (OPTSGUI_SENSITIVE | OPTSGUI_SHOW)) {
		if (option->type() == OPTTYPE_BOOL) {
			// Obtain value for setting sensitive
			gboolean ok;
			if (option->gui() == OPTGUI_TOGGLE)
				ok=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets[0]));
			else
				ok = ((Option_Bool *)option)->value ();
			// Get widgets to be set sensitive/shown/hidden
			std::set<std::string> gs;
			if (whattodo == 2)
				((Option_Bool *)option)->gui_sensitive (gs);
			else
				((Option_Bool *)option)->gui_show (gs);
			// Change widgets
			std::set<std::string>::iterator it = gs.begin ();
			while (it != gs.end ()) {
				GtkWidget *other = get_widget (it->c_str(), xml, file);
				it++;
				if (!other)
					continue;
				if (whattodo & OPTSGUI_SENSITIVE)
					gtk_widget_set_sensitive (other, ok);
				if (whattodo & OPTSGUI_SHOW)
					ok ? gtk_widget_show (other) : gtk_widget_hide (other);
			}
		}
	}
}

/**
 *  Get the description of the given group {\em group}.
 *
 *  @param  group Identifier of the group.
 */
std::string 
Options::group_help (guint group)
{
	if (groups_.find (group) == groups_.end ())
		return std::string("");
	return groups_[group]->help();
}

/**
 *  Get the name of the given group {\em group}.
 *
 *  @param  group Identifier of the group.
 */
std::string 
Options::group_name (guint group)
{
	if (groups_.find (group) == groups_.end ())
		return std::string("");
	return groups_[group]->name();
}

GtkWidget * 
Options::get_widget (const gchar *name, GladeXML *xml, const gchar *filename)
{
	if ((!name) || (!*name))
		return NULL;
	GtkWidget *widget = glade_xml_get_widget (xml, name);
	if (!widget)
		g_warning (_("Cannot find the specified widget (\"%s\")"
					 " within xml structure (\"%s\")"), name, filename);
	return widget;
}

Option * 
Options::find_option (const std::string &name, OptionType type)
{
	iterator opt = options_.find (name);
	if ((opt == options_.end()) || ((type != OPTTYPE_NONE)
									&& (opt->second->type() != type)))
		return NULL;
	return opt->second;
}
