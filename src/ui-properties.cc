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

#include "support.h"

#include <sstream>

#include "ui-preferences.h"
#include "ui-properties.h"
#include "mailbox.h"
#include "imap4.h"
#include "pop3.h"
#include "apop.h"


/**
 * "C" binding
 **/
extern "C" {
	void PROPERTIES_on_delay (GtkWidget *widget,
							  gpointer data)
	{
		PROPERTIES(data)->on_delay (widget);
	}

	void PROPERTIES_on_port (GtkWidget *widget,
							 gpointer data)
	{
		PROPERTIES(data)->on_port (widget);
	}

	void PROPERTIES_on_mailbox (GtkWidget *widget,
								gpointer data)
	{
		PROPERTIES(data)->on_mailbox (widget);
	}

	void PROPERTIES_on_type_changed (GtkAction *action,
									 gpointer   data)
	{
		PROPERTIES(data)->on_type_changed (action);
	}

	void PROPERTIES_on_auth_changed (GtkAction *action,
									 gpointer   data)
	{
		PROPERTIES(data)->on_auth_changed (action);
	}

	void PROPERTIES_on_browse_address (GtkWidget *widget,
									   gpointer data)
	{
		PROPERTIES(data)->on_browse_address (widget);
	}

	void PROPERTIES_on_browse_certificate (GtkWidget *widget,
										   gpointer data)
	{
		PROPERTIES(data)->on_browse_certificate (widget);
	}

}

Properties::Properties (Preferences *preferences) : GUI (GNUBIFF_DATADIR"/properties.glade")
{
	preferences_ = preferences;
	mailbox_ = 0;
	group_ = 0;
}

Properties::~Properties (void)
{
	GtkWidget *widget = get("dialog");
	if (GTK_IS_WIDGET (widget)) {
		hide ();
		gtk_widget_destroy (widget);
	}
	if (GTK_IS_OBJECT (xml_))
		g_object_unref (xml_);
}

gboolean
Properties::create (void)
{
	gboolean result = GUI::create();

	group_ = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (group_, get ("name"));
	gtk_size_group_add_widget (group_, get ("connection"));
	gtk_size_group_add_widget (group_, get ("authentication"));
	gtk_size_group_add_widget (group_, get ("certificate"));
	gtk_size_group_add_widget (group_, get ("delay"));
	gtk_size_group_add_widget (group_, get ("mailbox"));

	// Type menu
	GtkActionEntry type_entries[] = {
		{ "Autodetect", GTK_STOCK_DIALOG_QUESTION, "Autodetect",     0, "Autodetect type",   G_CALLBACK(PROPERTIES_on_type_changed)},
		{ "File",       GTK_STOCK_HOME,            "File or Folder", 0, "File, Mh, Maildir", G_CALLBACK(PROPERTIES_on_type_changed)},
		{ "Pop",        GTK_STOCK_NETWORK,         "Pop",            0, "Pop3 or Apop",      G_CALLBACK(PROPERTIES_on_type_changed)},
		{ "Imap",       GTK_STOCK_NETWORK,         "Imap",           0, "Imap4",             G_CALLBACK(PROPERTIES_on_type_changed)}
	};
	static const char *type_ui_description =
		"<ui>"
		"  <popup name='Type'>"
		"    <menuitem action='Autodetect'/>"
		"    <menuitem action='File'/>"
		"    <menuitem action='Pop'/>"
		"    <menuitem action='Imap'/>"
		"  </popup>"
		"</ui>";
	GtkActionGroup *action_group = gtk_action_group_new ("actions");
	gtk_action_group_add_actions (action_group, type_entries, G_N_ELEMENTS (type_entries), this);
	type_manager_ = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (type_manager_, action_group, 0);
	gtk_ui_manager_add_ui_from_string (type_manager_, type_ui_description, -1, 0);
	type_menu_ = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (type_menu_), gtk_ui_manager_get_widget (type_manager_, "/Type"));
	gtk_container_add (GTK_CONTAINER(get("type_container")), type_menu_);
	gtk_widget_show (type_menu_);


	// Authentication menu
	GtkActionEntry auth_entries[] = {
		{ "Autodetect",		GTK_STOCK_DIALOG_QUESTION, "Autodetect",
		  0, 0, G_CALLBACK(PROPERTIES_on_auth_changed)},
		{ "UserPass",		GTK_STOCK_DIALOG_WARNING, "User/Pass",
		  0, 0, G_CALLBACK(PROPERTIES_on_auth_changed)},
		{ "Apop",			GTK_STOCK_DIALOG_AUTHENTICATION, "Encrypted User/Pass (apop)",
		  0, 0, G_CALLBACK(PROPERTIES_on_auth_changed)},
		{ "SSL",			GTK_STOCK_DIALOG_AUTHENTICATION, "SSL",
		  0, 0, G_CALLBACK(PROPERTIES_on_auth_changed)},
		{ "Certificate",	GTK_STOCK_DIALOG_AUTHENTICATION, "SSL with certificate",
		  0, 0, G_CALLBACK(PROPERTIES_on_auth_changed)}
	};
	static const char *auth_ui_description =
		"<ui>"
		"  <popup name='Auth'>"
		"    <menuitem action='Autodetect'/>"
		"    <menuitem action='UserPass'/>"
		"    <menuitem action='Apop'/>"
		"    <menuitem action='SSL'/>"
		"    <menuitem action='Certificate'/>"
		"  </popup>"
		"</ui>";
	action_group = gtk_action_group_new ("actions");
	gtk_action_group_add_actions (action_group, auth_entries, G_N_ELEMENTS (auth_entries), this);
	auth_manager_ = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (auth_manager_, action_group, 0);
	gtk_ui_manager_add_ui_from_string (auth_manager_, auth_ui_description, -1, 0);
	auth_menu_ = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (auth_menu_), gtk_ui_manager_get_widget (auth_manager_, "/Auth"));
	gtk_container_add (GTK_CONTAINER(get("auth_container")), auth_menu_);
	gtk_widget_show (auth_menu_);

	return result;
}

void
Properties::select (Mailbox *mailbox)
{
	if (!mailbox) {
		hide();
		return;
	}

	mailbox_ = mailbox;
	selected_auth_ = -1;
	selected_type_ = -1;
	update_view ();
}

void
Properties::show (std::string name)
{
	if (!mailbox_)
		return;

	select (mailbox_);
	gtk_widget_show (get("dialog"));
}

// ========================================================================
//  Callbacks
// ========================================================================
void
Properties::on_delay (GtkWidget *widget)
{
	gint minutes = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(get("minutes_spin")));
	gint seconds = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(get("seconds_spin")));

	if ((minutes == 0) && (seconds < 5))
		gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("seconds_spin")), 5);
}

void
Properties::on_port (GtkWidget *widget)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (get("standard_port_radio"))))
		gtk_widget_set_sensitive (get("port_spin"), false);
	else
		gtk_widget_set_sensitive (get("port_spin"), true);
}

void
Properties::on_mailbox (GtkWidget *widget)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (get("standard_mailbox_radio"))))
		gtk_widget_set_sensitive (get("mailbox_entry"), false);
	else
		gtk_widget_set_sensitive (get("mailbox_entry"), true);
}

void
Properties::on_type_changed (GtkAction *action)
{
	std::string type = gtk_action_get_name (action);

	if (type == "Autodetect") {
		gtk_widget_set_sensitive (get("browse_address"), true);
		selected_type_ = TYPE_AUTODETECT;
		identity_view (false);
		details_view (false);
	}
	else if (type == "File") {
		gtk_widget_set_sensitive (get("browse_address"), true);
		selected_type_ = TYPE_LOCAL;
		identity_view (false);
		details_view (false);
	}
	else if (type == "Pop") {
		gtk_widget_set_sensitive (get("browse_address"), false);
		selected_type_ = TYPE_POP;
		identity_view (true);
		details_view (true);
		connection_view (true);
		auth_view (true);
		certificate_view (false);
		mailbox_view (false);
		delay_view (true);
	}
	else if (type == "Imap") {
		gtk_widget_set_sensitive (get("browse_address"), false);
		selected_type_ = TYPE_IMAP;
		identity_view (true);
		details_view (true);
		connection_view (true);
		auth_view (true);
		certificate_view (false);
		mailbox_view (true);
		delay_view (false);
	}
}

void
Properties::on_auth_changed (GtkAction *action)
{
	std::string auth = gtk_action_get_name (action);

	if (auth == "Autodetect") {
		selected_auth_ = AUTH_AUTODETECT;
		certificate_view (false);
	}
	else if (auth == "UserPass") {
		selected_auth_ = AUTH_USER_PASS;
		certificate_view (false);
	}
	else if (auth == "Apop") {
		selected_auth_ = AUTH_APOP;
		certificate_view (false);
	}
	else if (auth == "SSL") {
		selected_auth_ = AUTH_SSL;
		certificate_view (false);
	}
	else if (auth == "Certificate") {
		selected_auth_ = AUTH_CERTIFICATE;
		certificate_view (true);
	}
	else {
		selected_auth_ = AUTH_AUTODETECT;
		certificate_view (false);
	}
}

void
Properties::on_browse_address (GtkWidget *widget)
{
	browse (_("Browse for a file or folder"), "address_entry", true);
}

void
Properties::on_browse_certificate (GtkWidget *widget)
{
	browse (_("Browse for a certificate file"), "certificate_entry");
}

void
Properties::on_ok (GtkWidget *widget)
{
	on_apply (widget);
	preferences_->added(0);
	hide ();
}

void
Properties::on_apply (GtkWidget *widget)
{
	if (!mailbox_)
		return;

	mailbox_->name (gtk_entry_get_text (GTK_ENTRY (get("name_entry"))));
	// FIXME: need to parse address
	//	mailbox_->address (gtk_entry_get_text (GTK_ENTRY (get("address_entry"))));
	mailbox_->username (gtk_entry_get_text (GTK_ENTRY (get("username_entry"))));
	mailbox_->password (gtk_entry_get_text (GTK_ENTRY (get("password_entry"))));
	mailbox_->certificate (gtk_entry_get_text (GTK_ENTRY (get("certificate_entry"))));
	mailbox_->other_port ((guint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(get("port_spin"))));
	mailbox_->other_folder (gtk_entry_get_text (GTK_ENTRY (get("mailbox_entry"))));
	mailbox_->authentication (selected_auth_);
	gint minutes = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(get("minutes_spin")));
	gint seconds = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(get("seconds_spin")));
	mailbox_->delay (minutes*60+seconds);


	// Here we need to update or transform mailbox according to several criterion:
	//  - If type is autodetect we just set protocol to PROTOCOL_NONE and next mail
	//    check will lookup for mailbox format
	//  - If type is LOCAL, we also set protocol to PROTOCOL_NONE since we cannot
	//    yet foresee what is real mailbox format
	//

	// First case: type has been set to autodetect, we simply put procotol
	//             to PROTOCOL_NONE and lookup will be done automatically.
	if (selected_type_ == TYPE_AUTODETECT) {
		mailbox_->address (gtk_entry_get_text (GTK_ENTRY (get("address_entry"))));
		mailbox_->protocol (PROTOCOL_NONE);
	}

	// Second case: type has been set to local and protocol was already local, we
	//              need to know if address has been changed in order to force a
	//              lookup
	if (selected_type_ == TYPE_LOCAL) {
		if (((mailbox_->protocol() == PROTOCOL_FILE) ||
			 (mailbox_->protocol() == PROTOCOL_MH) ||
			 (mailbox_->protocol() == PROTOCOL_MAILDIR))) {
			// Address has changed -> force lookup
			if (mailbox_->address() != gtk_entry_get_text (GTK_ENTRY (get("address_entry"))))
				mailbox_->protocol (PROTOCOL_NONE);
			mailbox_->address (gtk_entry_get_text (GTK_ENTRY (get("address_entry"))));
		}
		// protocol was not local -> force lookup
		else {
			mailbox_->address (gtk_entry_get_text (GTK_ENTRY (get("address_entry"))));
			mailbox_->protocol (PROTOCOL_NONE);
		}
	}
	

	// Third case: type is set to imap
	else if (selected_type_ == TYPE_IMAP) {
		mailbox_->address (gtk_entry_get_text (GTK_ENTRY (get("address_entry"))));

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("standard_port_radio")))) {
			mailbox_->use_other_port (false);
			if ((selected_auth_ == AUTH_SSL) || (selected_auth_ == AUTH_CERTIFICATE))
				mailbox_->port (993);
			else
				mailbox_->port (143);
		}
		else {
			mailbox_->use_other_port (true);
			mailbox_->port (mailbox_->other_port());
		}

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("standard_mailbox_radio")))) {
			mailbox_->use_other_folder (false);
			mailbox_->folder ("INBOX");
		}
		else {
			mailbox_->use_other_folder (true);
			mailbox_->folder (mailbox_->other_folder());
		}
		
		// Protocol was not imap4, we change mailbox
		if (mailbox_->protocol() != PROTOCOL_IMAP4) {
			Mailbox *mailbox = new Imap4 (*mailbox_);
			preferences_->biff()->replace (mailbox_, mailbox);
		}
	}


	// Fourth case: type is set to pop
	else if (selected_type_ == TYPE_POP) {
		mailbox_->address (gtk_entry_get_text (GTK_ENTRY (get("address_entry"))));
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("standard_port_radio")))) {
			mailbox_->use_other_port (false);
			if ((selected_auth_ == AUTH_SSL) || (selected_auth_ == AUTH_CERTIFICATE))
				mailbox_->port (995);
			else
				mailbox_->port (110);
		}
		else {
			mailbox_->use_other_port (true);
			mailbox_->port (mailbox_->other_port());
		}

		if ((mailbox_->protocol() != PROTOCOL_APOP) && (selected_auth_ == AUTH_APOP)) {
			Mailbox *mailbox = new Apop (*mailbox_);
			preferences_->biff()->replace (mailbox_, mailbox);
		}
		else if ((mailbox_->protocol() != PROTOCOL_POP3)) {
			Mailbox *mailbox = new Pop3 (*mailbox_);
			preferences_->biff()->replace (mailbox_, mailbox);
		}
	}


	mailbox_->address (gtk_entry_get_text (GTK_ENTRY (get("address_entry"))));

	preferences_->synchronize();	
}

void
Properties::on_cancel (GtkWidget *widget)
{
	hide ();
	Preferences *prefs = preferences_;
	if (prefs->added()) {
		prefs->biff()->remove (prefs->biff()->get(prefs->added()->uin()));
		prefs->added(0);
		prefs->synchronize ();		
	}
}

gboolean
Properties::on_destroy (GtkWidget *widget,
						GdkEvent *event)
{
	on_cancel (0);
	return TRUE;
}

gboolean
Properties::on_delete	(GtkWidget *widget,
						 GdkEvent *event)
{
	on_cancel (0);
	return TRUE;
}


// ========================================================================
//
//  GUI modification
//
// ========================================================================
void
Properties::update_view (void)
{
	// in case there is no mailbox to update view from
	if (!mailbox_)
		return;

	gtk_entry_set_text (GTK_ENTRY (get("name_entry")), mailbox_->name().c_str());
	gtk_entry_set_text (GTK_ENTRY (get("address_entry")), mailbox_->address().c_str());
	gtk_entry_set_text (GTK_ENTRY (get("username_entry")), mailbox_->username().c_str());
	gtk_entry_set_text (GTK_ENTRY (get("password_entry")), mailbox_->password().c_str());
	gtk_entry_set_text (GTK_ENTRY (get("certificate_entry")), mailbox_->certificate().c_str());

	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("port_spin")), mailbox_->other_port());
	if (mailbox_->use_other_port()) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("standard_port_radio")), false);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("other_port_radio")), true);
		gtk_widget_set_sensitive (get("port_spin"), true);
	}
	else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("standard_port_radio")), true);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("other_port_radio")), false);
		gtk_widget_set_sensitive (get("port_spin"), false);
	}

	gtk_entry_set_text (GTK_ENTRY (get("mailbox_entry")), mailbox_->other_folder().c_str());
	if (mailbox_->use_other_folder()) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("standard_mailbox_radio")), false);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("other_mailbox_radio")), true);
		gtk_widget_set_sensitive (get("mailbox_entry"), true);
	}
	else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("standard_mailbox_radio")), true);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("other_mailbox_radio")), false);
		gtk_widget_set_sensitive (get("mailbox_entry"), false);
	}

	gint delay = mailbox_->delay ();
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("seconds_spin")), delay%60);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("minutes_spin")), delay/60);

	type_view ();
}

void
Properties::type_view (void)
{
	// in case there is no mailbox to get type from
	if (!mailbox_)
		return;

	gtk_widget_set_sensitive (get("browse_address"), true);
	identity_view (false);
	details_view (false);
	connection_view (true);
	auth_view (false);
	certificate_view (false);
	mailbox_view (false);
	delay_view (false);

	guint protocol = mailbox_->protocol ();
	selected_type_ = TYPE_AUTODETECT;

	if ((protocol == PROTOCOL_FILE) || (protocol == PROTOCOL_MH) || (protocol == PROTOCOL_MAILDIR)) {
		selected_type_ = TYPE_LOCAL;
	}
	else if ((protocol == PROTOCOL_POP3) || (protocol == PROTOCOL_APOP)) {
		selected_type_ = TYPE_POP;
		gtk_widget_set_sensitive (get("browse_address"), false);
		identity_view (true);
		details_view (true);
		delay_view (true);
		auth_view (true);
	}
	else if ((protocol == PROTOCOL_IMAP4)) {
		selected_type_ = TYPE_IMAP;
		gtk_widget_set_sensitive (get("browse_address"), false);
		identity_view (true);
		details_view (true);
		delay_view (false);
		mailbox_view (true);
		auth_view (true);
	}

	gtk_option_menu_set_history (GTK_OPTION_MENU (type_menu_), selected_type_);
}

void
Properties::identity_view (gboolean visible)
{
	if (visible) {
		gtk_widget_show (get("username"));
		gtk_widget_show (get("username_entry"));
		gtk_widget_show (get("password"));
		gtk_widget_show (get("password_entry"));
	}
	else {
		gtk_widget_hide (get("username"));
		gtk_widget_hide (get("username_entry"));
		gtk_widget_hide (get("password"));
		gtk_widget_hide (get("password_entry"));
	}
}

void
Properties::details_view (gboolean visible)
{
	if (visible) {
		if (!GTK_WIDGET_VISIBLE (get("details_expander"))) {
			gtk_widget_show (get("details_expander"));
			gtk_size_group_add_widget (group_, get ("connection"));
			gtk_size_group_add_widget (group_, get ("authentication"));
			gtk_size_group_add_widget (group_, get ("certificate"));
			gtk_size_group_add_widget (group_, get ("delay"));
			gtk_size_group_add_widget (group_, get ("mailbox"));
		}
	}
	else {
		if (GTK_WIDGET_VISIBLE (get("details_expander"))) {
			gtk_widget_hide (get("details_expander"));
			gtk_size_group_remove_widget (group_, get ("connection"));
			gtk_size_group_remove_widget (group_, get ("authentication"));
			gtk_size_group_remove_widget (group_, get ("certificate"));
			gtk_size_group_remove_widget (group_, get ("delay"));
			gtk_size_group_remove_widget (group_, get ("mailbox"));
		}
	}
}

void
Properties::connection_view (gboolean visible)
{
	if (visible)
		gtk_widget_show (get("connection_alignment"));
	else
		gtk_widget_hide (get("connection_alignment"));
}

void
Properties::auth_view (gboolean visible)
{
	if (visible)
		gtk_widget_show (get("authentication_alignment"));
	else {
		gtk_widget_hide (get("authentication_alignment"));
		return;
	}

#ifdef HAVE_CRYPTO
	if (selected_type_ == TYPE_POP)
		gtk_widget_set_sensitive (gtk_ui_manager_get_widget (auth_manager_, "/Auth/Apop"), true);
	else
		gtk_widget_set_sensitive (gtk_ui_manager_get_widget (auth_manager_, "/Auth/Apop"), false);
#else
		gtk_widget_set_sensitive (gtk_ui_manager_get_widget (auth_manager_, "/Auth/Apop"), false);
#endif

#ifdef HAVE_LIBSSL
		gtk_widget_set_sensitive (gtk_ui_manager_get_widget (auth_manager_, "/Auth/SSL"), true);
		gtk_widget_set_sensitive (gtk_ui_manager_get_widget (auth_manager_, "/Auth/Certificate"), true);
#else
		gtk_widget_set_sensitive (gtk_ui_manager_get_widget (auth_manager_, "/Auth/SSL"), false);
		gtk_widget_set_sensitive (gtk_ui_manager_get_widget (auth_manager_, "/Auth/Certificate"), false);
#endif

	
	// Get authentication method
	gint auth = AUTH_AUTODETECT;

	if (selected_auth_ == -1)
		auth = mailbox_->authentication();
	else
		auth = selected_auth_;

	// Now we check what are available authentication methods
#ifndef HAVE_CRYPTO
	if (auth == AUTH_APOP)
		auth = AUTH_USER_PASS;
#endif

#ifndef HAVE_LIBSSL
	if ((auth == AUTH_SSL) || (auth == AUTH_CERTIFICATE))
#  ifdef HAVE_CRYPTO
		if (selected_type == TYPE_POP)
			auth = AUTH_APOP;
#  else
		auth = AUTH_USER_PASS;
#  endif
#endif


	// Just in case "encrypted user pass" was selected and we just switch
	// to an imap mailbox (where apop encryption is not possible)
	if ((auth == AUTH_APOP) && (selected_type_ != TYPE_POP)) {
#ifdef HAVE_LIBSSL
		auth = AUTH_SSL;
#else
		auth = AUTH_USER_PASS;
#endif
	}

	gtk_option_menu_set_history (GTK_OPTION_MENU (auth_menu_), auth);
	certificate_view (auth == AUTH_CERTIFICATE);
	selected_auth_ = auth;
}

void
Properties::certificate_view (gboolean visible)
{
	if (visible)
		gtk_widget_show (get("certificate_alignment"));
	else
		gtk_widget_hide (get("certificate_alignment"));
}

void
Properties::mailbox_view (gboolean visible)
{
	if (visible)
		gtk_widget_show (get("mailbox_alignment"));
	else
		gtk_widget_hide (get("mailbox_alignment"));
}

void
Properties::delay_view (gboolean visible)
{
	if (visible)
		gtk_widget_show (get("delay_alignment"));
	else
		gtk_widget_hide (get("delay_alignment"));
}
