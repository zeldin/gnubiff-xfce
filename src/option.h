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

#ifndef __OPTION_H__
#define __OPTION_H__

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif
#include <glib.h>
#include <gtk/gtk.h>
#include <map>
#include <set>
#include <string>
#include <vector>

const guint OPTGRP_NONE	= 0;
const guint OPTGRP_ALL	= (guint)-1;

/// No flags
const guint OPTFLG_NONE = 0;
/// Shall the string be tested for being the name of a regular file? (string)
const guint OPTFLG_TEST_FILE = 1;
/// Are only the given values allowed? (guint)
const guint OPTFLG_ID_INT_STRICT = 2;
/// The option's value is fixed.
const guint OPTFLG_FIXED = 4;
/// The option's value is set automatically.
const guint OPTFLG_AUTO = 8;
/// The option will not be saved
const guint OPTFLG_NOSAVE = 16;
/** If this option is changed, other options or variables have to be updated
 *  automatically. This flag is only checked when accessing to the values
 *  via the Options class, not when accessing to the option directly.*/
const guint OPTFLG_CHANGE = 32;
/** If this option is read, it has to be updated before, because it depends
 *  on the values of other options or variables. This flag is only checked
 *  when accessing to the values via the Options class, not when accessing to
 *  the option directly.*/
const guint OPTFLG_UPDATE = 64;
/// The option is a space separated list of strings (string)
const guint OPTFLG_STRINGLIST = 128;

/// Option cannot be changed by the user
const guint OPTFLG_USER_NO_CHANGE = OPTFLG_FIXED | OPTFLG_AUTO;

enum OptionType {OPTTYPE_NONE = 0, OPTTYPE_UINT, OPTTYPE_STRING, OPTTYPE_BOOL};

enum OptionGUI {OPTGUI_NONE = 0, OPTGUI_TOGGLE, OPTGUI_SPIN, OPTGUI_ENTRY,
				OPTGUI_FONT, OPTGUI_RADIO};

class Option {
public:
	Option ();
	Option (std::string name, guint group, std::string help,
			guint flags = OPTFLG_NONE, OptionGUI gui = OPTGUI_NONE,
			std::string gui_name = std::string(""));
	/// Destructor.
	virtual ~Option () {};

	virtual std::string type_string (void) {return "none";};
	virtual std::string to_string (void) {return std::string("");};
	virtual gboolean from_string (const std::string &value) {return false;};
	virtual void set_gui (std::vector<GtkWidget *> &widgets) {};
	virtual void get_gui (std::vector<GtkWidget *> &widgets) {};
	/**
	 *  Reset the option to the default value.
	 *
	 *  Remark: Instead of calling this function directly better call
	 *  Options::reset(). This function handles flags like OPTFLG_CHANGE!
	 */
	virtual void reset (void) {};
	virtual gboolean is_default (void) {return false;};
	virtual std::string default_string (void) {return std::string("");};
	virtual Option *copy (void) {return new Option(*this);};
	std::string flags_string (std::string sep = std::string("; "));

	/// Access function to Option::flags_
	guint flags (void) const {return flags_;}
	/// Access function to Option::group_
	guint group (void) const {return group_;}
	/// Access function to Option::gui_
	OptionGUI gui (void) const {return gui_;}
	/// Access function to Option::gui_name_
	std::string gui_name (void) const {return gui_name_;}
	/// Access function to Option::help
	std::string help (void) const {return help_;}
	/// Access function to Option::name_
	std::string name (void) const {return name_;}
	/// Access function to Option::type_
	OptionType type (void) const {return type_;}
protected:
	/// Name of the option
	std::string name_;
	/// Type of the option
	OptionType type_;
	/// Group to which the option belongs
	guint group_;
	/// Short description of the option
	std::string help_;
	/** Type of the GUI element corresponding to this option in the preferences
	 *  dialog. */
	OptionGUI gui_;
	/** Name of the GUI element corresponding to this option in the preferences
	 *  dialog. */
	std::string gui_name_;
	/// Flags
	guint flags_;
};

class Option_UInt : public Option {
public:
	Option_UInt (std::string name, guint group, std::string help,
				 guint def, guint flags, const guint array_int[],
				 const gchar *array_id[], OptionGUI gui = OPTGUI_NONE,
				 std::string gui_name = std::string(""));
	Option_UInt (std::string name, guint group, std::string help,
				 guint def, guint flags = OPTFLG_NONE,
				 OptionGUI gui = OPTGUI_NONE,
				 std::string gui_name = std::string(""));

	std::string type_string (void) {return ((flags_ & OPTFLG_ID_INT_STRICT) ? "enum" : "unsigned int");};
	std::string to_string (void);
	std::string default_string (void);
	gboolean from_string (const std::string &value);
	void get_gui (std::vector<GtkWidget *> &widgets);
	void set_gui (std::vector<GtkWidget *> &widgets);
	void reset (void);
	std::string allowed_ids (std::string sep = std::string(" "));
	gboolean is_default (void) {return value_ == default_;};
	Option *copy (void) {return new Option_UInt(*this);};

	/// Access function to Option_UInt::value_
	guint value (void) {return value_;};
	/// Access function to Option_UInt::value_
	void value (guint val) {value_ = val;};

	const std::string value_to_string (guint val);
	guint string_to_value (const std::string &str);
protected:
	/// Value of the option
	guint value_;
	/// Default value of the option
	guint default_;
	/// Map for converting identifiers to values
	std::map<std::string, guint> id_int_;
	/// Map for converting values to identifiers
	std::map<guint, std::string> int_id_;
};

class Option_Bool : public Option_UInt {
public:
	Option_Bool (std::string name, guint group, std::string help, gboolean def,
				 guint flags = OPTFLG_NONE, OptionGUI gui = OPTGUI_NONE,
				 std::string gui_name = std::string(""),
				 const gchar *gui_sensitive[] = NULL,
				 const gchar *gui_show[] = NULL);

	Option *copy (void) {return new Option_Bool(*this);};
	std::string type_string (void) {return "bool";};

	/// Access function to Option_UInt::value_
	gboolean value (void) {return (gboolean)value_;};
	/// Access function to Option_UInt::value_
	void value (gboolean val) {value_ = (guint)val;};
	/// Access function to Option_UInt::gui_sensitive_
	void gui_sensitive (std::set<std::string> &gs) {gs = gui_sensitive_;};
	/// Access function to Option_UInt::gui_sensitive_neg_
	void gui_show (std::set<std::string> &gs) {gs = gui_show_;};
private:
	static const gchar *ids_[3];
	static const guint ints_[3];
	/// GUI elements that are sensitive to this boolean
	std::set<std::string> gui_sensitive_;
	/// GUI elements that are shown/hidden depending on this boolean
	std::set<std::string> gui_show_;
};

class Option_String : public Option {
public:
	Option_String (std::string name, guint group, std::string help,
				   std::string def, guint flags = OPTFLG_NONE,
				   OptionGUI gui = OPTGUI_NONE,
				   std::string gui_name = std::string(""));

	std::string type_string (void) {return ((flags_ & OPTFLG_STRINGLIST) ? "list (strings)" : "string");};
	std::string to_string (void);
	std::string default_string (void);
	gboolean from_string (const std::string &value);
	void get_gui (std::vector<GtkWidget *> &widgets);
	void set_gui (std::vector<GtkWidget *> &widgets);
	void reset (void);
	gboolean is_default (void) {return value_ == default_;};
	Option *copy (void) {return new Option_String(*this);};

	void set_values (const std::set<std::string> &values, gboolean empty=true);
	void get_values (std::set<std::string> &values, gboolean empty = true);
	void get_vector (std::vector<guint> &vector, gchar sep = ' ',
					 gboolean empty = true);

	/// Access function to Option_UInt::value_
	std::string value (void) {return value_;};
	/// Access function to Option_UInt::value_
	void value (const std::string val) {from_string (val);};
protected:
	/// Value of the option
	std::string value_;
	/// Default value of the option
	std::string default_;
};

class Option_Group {
public:
	Option_Group ();
	Option_Group (std::string name, guint id, std::string help);
	/// Access function to Option::group_
	guint id (void) {return id_;}
	/// Access function to Option::help_
	std::string help (void) {return help_;}
	/// Access function to Option::name_
	std::string name (void) {return name_;}
protected:
	/// Name of the group
	std::string name_;
	/// Group ID
	guint id_;
	/// Short description of the group's options
	std::string help_;
};

#endif
