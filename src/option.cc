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
// Short         : One option for gnubiff
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#include <algorithm>
#include <sstream>
#include "option.h"

Option::Option ()
{
	name_		= std::string("");
	type_		= OPTTYPE_NONE;
	group_		= OPTGRP_NONE;
	help_		= std::string("");
	gui_		= OPTGUI_NONE;
	gui_name_	= std::string("");
	flags_		= OPTFLG_NONE;
}

Option::Option (std::string name, guint group, std::string help, guint flags,
				OptionGUI gui, std::string gui_name)
{
	name_		= name;
	type_		= OPTTYPE_NONE;
	group_		= group;
	help_		= help;
	gui_		= gui;
	gui_name_	= gui_name;
	flags_		= flags;
}

/**
 *  Return all flags as a human readable list in a string.
 *
 *  @param  sep  Separator between list elements, default is "; "
 *  @return      List of flags
 */
std::string 
Option::flags_string (std::string sep)
{
	std::string result;

	if (flags_ == OPTFLG_NONE)
		return "none";
	if (flags_ & OPTFLG_TEST_FILE)
		result += "only regular filenames allowed" + sep;
	if (flags_ & OPTFLG_ID_INT_STRICT)
		result += "only given identifiers allowed" + sep;
	if (flags_ & OPTFLG_FIXED)
		result += "option has fixed value" + sep;
	if (flags_ & OPTFLG_AUTO)
		result += "option is set automatically" + sep;
	if (flags_ & OPTFLG_NOSAVE)
		result += "option is not saved to config file" + sep;
	if (flags_ & OPTFLG_CHANGE)
		result += "editing this option may change other options" + sep;
	if (flags_ & OPTFLG_UPDATE)
		result += "option is automatically updated each time it is read" + sep;
	if (flags_ & OPTFLG_STRINGLIST)
		result += "option is a list" + sep;
	if (flags_ & OPTFLG_NOSHOW)
		result += "option is not to be shown" + sep;
	return result.substr (0, result.size()-sep.size());
}

Option_UInt::Option_UInt (std::string name, guint group, std::string help,
						  guint def, guint flags, const guint array_int[],
						  const gchar *array_id[], 
						  OptionGUI gui, std::string gui_name)
						 : Option (name, group, help, flags, gui, gui_name)
{
	type_			= OPTTYPE_UINT;
	value_			= def;
	default_		= def;
	if ((array_int != NULL) && (array_id != NULL)) {
		guint i = 0;
		while  (array_id[i] != NULL) {
			id_int_[std::string(array_id[i])] = array_int[i];
			int_id_[array_int[i]] = std::string(array_id[i]);
			i++;
		}
	}
}

Option_UInt::Option_UInt (std::string name, guint group, std::string help,
						  guint def, guint flags, OptionGUI gui,
						  std::string gui_name)
						 : Option (name, group, help, flags, gui, gui_name)
{
	type_			= OPTTYPE_UINT;
	value_			= def;
	default_		= def;
}

std::string 
Option_UInt::to_string (void)
{
	// Test whether there is an identifier for this value
	if (int_id_.find (value_) != int_id_.end ())
		return int_id_[value_];

	std::stringstream ss;
	ss << value_;
	return ss.str();
}

std::string 
Option_UInt::default_string (void)
{
	// Test whether there is an identifier for this value
	if (int_id_.find (default_) != int_id_.end ())
		return int_id_[default_];

	std::stringstream ss;
	ss << default_;
	return ss.str();
}

gboolean 
Option_UInt::from_string (const std::string &value)
{
	// Test whether the string is a known identifier
	if (id_int_.find (value) != id_int_.end ()) {
		value_ = id_int_[value];
		return true;
	}

	guint tmp;
	std::stringstream ss (value);
	if ((!(ss >> tmp)) || ((flags_ & OPTFLG_ID_INT_STRICT)
						   && (int_id_.find (tmp) == int_id_.end ()))) {
		value_ = default_;
		return false;
	}
	value_ = tmp;
	return true;
}

void 
Option_UInt::get_gui (std::vector<GtkWidget *> &widgets)
{
	switch (gui_) {
	case OPTGUI_SPIN:
		if (widgets[0])
		value_ = (guint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widgets[0]));
		break;
	case OPTGUI_TOGGLE:
		if (widgets[0])
			value_=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets[0]));
		break;
	case OPTGUI_RADIO:
		for (guint i = 0; i < widgets.size(); i++) {
			if (!widgets[i])
				continue;
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widgets[i]))) {
				value_ = i;
				break;
			}
		}
		break;
	default:
		break;
	}
}

void 
Option_UInt::set_gui (std::vector<GtkWidget *> &widgets)
{
	switch (gui_) {
	case OPTGUI_SPIN:
		if (widgets[0])
			gtk_spin_button_set_value (GTK_SPIN_BUTTON(widgets[0]), value_);
		break;
	case OPTGUI_TOGGLE:
		if (widgets[0])
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets[0]),value_);
		break;
	case OPTGUI_RADIO:
		for (guint i = 0; i < widgets.size(); i++) {
			if (!widgets[i])
				continue;
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(widgets[i]),
										  i == value_);
		}
		break;
	default:
		break;
	}
}

/**
 *  Reset the option to the default value.
 *
 *  Remark: Instead of calling this function directly better call
 *  Options::reset(). This function handles flags like OPTFLG_CHANGE!
 */
void 
Option_UInt::reset (void)
{
	value_ = default_;
}

/**
 *  Return all identifiers to which this option can be set. These will be
 *  returned as a {\em sep} separated list in a string.
 *
 *  @param  sep  List separator, default is " "
 *  @return      List (in a string) with all identifiers
 */
std::string 
Option_UInt::allowed_ids (std::string sep)
{
	std::string result;

	std::map<guint, std::string>::iterator it = int_id_.begin ();
	while (it != int_id_.end ()) {
		if (it != int_id_.begin ())
			result += sep;
		result += (it++)->second;
	}
	return result;
}

const std::string 
Option_UInt::value_to_string (guint val)
{
	if (int_id_.find (val) != int_id_.end ())
		return int_id_[val];
	return std::string("");
}

guint 
Option_UInt::string_to_value (const std::string &str)
{
	if (id_int_.find (str) != id_int_.end ())
		return id_int_[str];
	return 0;
}

const gchar *Option_Bool::ids_[3] = {"false", "true", NULL};
const guint Option_Bool::ints_[3] = {0, 1, 0};

Option_Bool::Option_Bool (std::string name, guint group, std::string help,
						  gboolean def, guint flags, OptionGUI gui,
						  std::string gui_name, const gchar *gui_sensitive[],
						  const gchar *gui_show[])
						 : Option_UInt (name, group, help, def,
										flags | OPTFLG_ID_INT_STRICT, ints_,
										ids_, gui, gui_name)
{
	type_ = OPTTYPE_BOOL;
	guint i = 0;
	if (gui_sensitive)
		while (gui_sensitive[i] != NULL)
			gui_sensitive_.insert (std::string(gui_sensitive[i++]));
	if (gui_show)
		while (gui_show[i] != NULL)
			gui_show_.insert (std::string(gui_show[i++]));
}

Option_String::Option_String (std::string name, guint group, std::string help,
							  std::string def, guint flags, OptionGUI gui,
							  std::string gui_name)
							 : Option (name, group, help, flags, gui, gui_name)
{
	type_		= OPTTYPE_STRING;
	value_		= def;
	default_	= def;
}

std::string 
Option_String::to_string (void)
{
	return value_;
}

std::string 
Option_String::default_string (void)
{
	return default_;
}

gboolean 
Option_String::from_string (const std::string &value)
{
	if (flags_ & OPTFLG_TEST_FILE)
		if (!g_file_test (value.c_str (), G_FILE_TEST_EXISTS))
			return false;
	value_ = value;
	return true;
}

/**
 *  Set the value of the option. This is done by creating a space separated
 *  list of all the strings in the set {\em values}. If {\em empty} is true
 *  the old value of the option will be overwritten, otherwise the new
 *  strings are appended. Empty strings will not be stored.
 *
 *  Note: Do not use this function directly, use the function
 *  Options::set_values() instead. Then the OPTFLG_CHANGE flag will be
 *  respected.
 *
 *  Note: The character '\' is used as escape character, so "\x" as substring
 *  in the stored option will be transformed to 'x' when obtaining the values
 *  via Option_String::get_values() for any character 'x'. This is needed for
 *  storing spaces and '\' itself.
 *
 *  @param  values         New strings to add to the option's value
 *  @param  empty          Shall the option be erased before appending (the
 *                         default is true)?
 */
void 
Option_String::set_values (const std::set<std::string> &values, gboolean empty)
{
	std::set<std::string>::iterator i = values.begin ();
	if (empty)
		value_ = std::string("");
	while (i != values.end()) {
		std::string str = *(i++);
		std::string::size_type len = str.size ();
		if (len == 0)
			continue;
		for (std::string::size_type j = 0; j < len ; j++) {
			if ((str[j] == ' ') || (str[j] == '\\'))
				value_ += '\\';
			value_ += str[j];
		}
		value_ += ' ';
	}
}

/**
 *  Get the value of the option. The option's value is
 *  treated as a space separated list of strings. Each string in this list
 *  is returned in the set {\em var}. If {\em empty} is true the this set
 *  will be emptied before obtaining the values, otherwise the new strings
 *  are inserted in {\em var} in addition to the strings that are in this set
 *  before calling this function.
 *
 *  Note: Do not use this function directly, use the function
 *  Options::get_values() instead. Then the OPTFLG_UPDATE flag will be
 *  respected.
 *
 *  @param  values         Set in which the strings will be returned
 *  @param  empty          Shall the set {\em var} be erased before inserting
 *                         the strings (the default is true)?
 *
 *  @see See the description of Option_String::set_values() for the handling
 *       of the '\' character.
 */
void 
Option_String::get_values (std::set<std::string> &values, gboolean empty)
{
	if (empty)
		values.clear ();

	std::string tmp;
	std::string::size_type len = value_.size();

	std::string::size_type pos = 0;
	while (pos < len) {
		// Remove spaces
		while ((pos < len) && (value_[pos] == ' '))
			pos++;

		// Get quoted string
		if (pos < len) {
			if (get_quotedstring (value_, tmp, pos, ' ', false, true))
				values.insert (tmp);
			else
				break; // we stop if there is an error
		}
	}
}

void 
Option_String::get_vector (std::vector<guint> &vector,gchar sep,gboolean empty)
{
	if (empty)
		vector.clear ();

	std::string line = value_;
	if (sep != ' ')
		std::replace (line.begin(), line.end(), sep, ' ');

	std::stringstream ss (line);
	guint tmp;
	while (ss >> tmp)
		vector.push_back (tmp);
}

void 
Option_String::get_gui (std::vector<GtkWidget *> &widgets)
{
	switch (gui_) {
	case OPTGUI_ENTRY:
		if (widgets[0])
			value_ = gtk_entry_get_text (GTK_ENTRY (widgets[0]));
		break;
	case OPTGUI_FONT:
		if (widgets[0])
			value_=gtk_font_button_get_font_name (GTK_FONT_BUTTON(widgets[0]));
		break;
	default:
		break;
	}
}

void 
Option_String::set_gui (std::vector<GtkWidget *> &widgets)
{
	switch (gui_) {
	case OPTGUI_ENTRY:
		if (widgets[0])
			gtk_entry_set_text (GTK_ENTRY (widgets[0]), value_.c_str());
		break;
	case OPTGUI_FONT:
		if (widgets[0])
			gtk_font_button_set_font_name (GTK_FONT_BUTTON (widgets[0]),
										   value_.c_str());
		break;
	default:
		break;
	}
}

/**
 *  Reset the option to the default value.
 *
 *  Remark: Instead of calling this function directly better call
 *  Options::reset(). This function handles flags like OPTFLG_CHANGE!
 */
void 
Option_String::reset (void)
{
	value_ = default_;
}

Option_Group::Option_Group ()
{
	help_	= std::string("");
	id_		= OPTGRP_NONE;
	name_	= std::string("");
}

Option_Group::Option_Group (std::string name, guint id, std::string help)
{
	help_	= help;
	id_		= id;
	name_	= name;
}
