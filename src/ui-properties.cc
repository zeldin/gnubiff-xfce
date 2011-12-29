// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2011 Nicolas Rougier, 2004-2011 Robert Sowada
//
// This program is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

	void PROPERTIES_on_type_changed (GtkComboBox *cbox,
									 gpointer   data)
	{
		PROPERTIES(data)->on_type_changed ();
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
Properties::create (gpointer callbackdata)
{
	gboolean result = GUI::create(this);

	group_ = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (group_, get ("name"));
	gtk_size_group_add_widget (group_, get ("connection"));
	gtk_size_group_add_widget (group_, get ("authentication"));
	gtk_size_group_add_widget (group_, get ("certificate"));
	gtk_size_group_add_widget (group_, get ("delay"));
	gtk_size_group_add_widget (group_, get ("mailbox"));

    // Type menu
    type_cbox_ = GTK_COMBO_BOX (gtk_combo_box_new_text ());
    gtk_combo_box_append_text (type_cbox_, _("Autodetect"));
    gtk_combo_box_append_text (type_cbox_, _("File or Folder"));
    gtk_combo_box_append_text (type_cbox_, "Pop");
    gtk_combo_box_append_text (type_cbox_, "Imap");
	gtk_container_add (GTK_CONTAINER (get("type_container")),
                       GTK_WIDGET (type_cbox_));
	gtk_widget_show (GTK_WIDGET (type_cbox_));
    g_signal_connect (G_OBJECT (type_cbox_), "changed",
                      G_CALLBACK (PROPERTIES_on_type_changed), this);

	// Authentication menu
	GtkActionEntry auth_entries[] = {
		{ "Autodetect",		GTK_STOCK_DIALOG_QUESTION, _("Autodetect"),
		  0, 0, G_CALLBACK(PROPERTIES_on_auth_changed)},
		{ "UserPass",		GTK_STOCK_DIALOG_WARNING, _("User/Pass"),
		  0, 0, G_CALLBACK(PROPERTIES_on_auth_changed)},
		{ "Apop",			GTK_STOCK_DIALOG_AUTHENTICATION, _("Encrypted User/Pass (apop)"),
		  0, 0, G_CALLBACK(PROPERTIES_on_auth_changed)},
		{ "SSL",			GTK_STOCK_DIALOG_AUTHENTICATION, "SSL",
		  0, 0, G_CALLBACK(PROPERTIES_on_auth_changed)},
		{ "Certificate",	GTK_STOCK_DIALOG_AUTHENTICATION, _("SSL with certificate"),
		  0, 0, G_CALLBACK(PROPERTIES_on_auth_changed)},
		{ "TLS",			GTK_STOCK_DIALOG_AUTHENTICATION, "TLS",
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
		"    <menuitem action='TLS'/>"
		"  </popup>"
		"</ui>";
	GtkActionGroup *action_group = gtk_action_group_new ("actions");
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
	selected_auth_ = mailbox->authentication();
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
Properties::on_type_changed (void)
{
	selected_type_ = gtk_combo_box_get_active (type_cbox_);

    switch (selected_type_) {
    case TYPE_AUTODETECT:
 		gtk_widget_set_sensitive (get ("browse_address"), true);
 		identity_view (false);
 		details_view (false);
        break;
    case TYPE_LOCAL:
 		gtk_widget_set_sensitive (get("browse_address"), true);
 		identity_view (false);
 		details_view (false);
        break;
    case TYPE_POP:
 		gtk_widget_set_sensitive (get("browse_address"), false);
 		identity_view (true);
 		details_view (true);
 		auth_view (true);
 		connection_view (true);
 		certificate_view (false);
 		mailbox_view (false);
 		delay_view (true);
        break;
    case TYPE_IMAP:
 		gtk_widget_set_sensitive (get("browse_address"), false);
 		identity_view (true);
 		details_view (true);
 		auth_view (true);
 		connection_view (true);
 		certificate_view (false);
 		mailbox_view (true);
 		delay_view (true);
        break;
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
	else if (auth == "TLS") {
		selected_auth_ = AUTH_TLS;
		certificate_view (true);
	}
	else {
		selected_auth_ = AUTH_AUTODETECT;
		certificate_view (false);
	}
	// Maybe the standard port has changed: Update the displayed connection
	// details
	connection_view (true);
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

	// Save old address for comparing
	std::string oldaddress = mailbox_->address ();

	// Retrieve all values of the options from the GUI elements
	mailbox_->update_gui (OPTSGUI_GET, OPTGRP_MAILBOX, xml_, filename_);

	mailbox_->authentication (selected_auth_);

	// Here we need to update or transform mailbox according to several criterion:
	//  - If type is autodetect we just set protocol to PROTOCOL_NONE and next mail
	//    check will lookup for mailbox format
	//  - If type is LOCAL: See below

	// First case: type has been set to autodetect, we simply put procotol
	//             to PROTOCOL_NONE and lookup will be done automatically.
	if (selected_type_ == TYPE_AUTODETECT) {
		mailbox_->protocol (PROTOCOL_NONE);
		Mailbox *mailbox = new Mailbox (*mailbox_);
		preferences_->biff()->replace_mailbox (mailbox_, mailbox);
	}

	// Second case: type has been set to local
	if (selected_type_ == TYPE_LOCAL) {
		Mailbox *mailbox=NULL;

		if (((mailbox_->protocol() != PROTOCOL_FILE) &&
			 (mailbox_->protocol() != PROTOCOL_MH) &&
			 (mailbox_->protocol() != PROTOCOL_MH_BASIC) &&
			 (mailbox_->protocol() != PROTOCOL_MH_SYLPHEED) &&
			 (mailbox_->protocol() != PROTOCOL_MAILDIR))
			|| (mailbox_->address() == oldaddress))
		{
			// Something changed. Try to determine type now. This allows
			// setting the right mailbox now, so the properties dialog will
			// show the correct mailbox type if the user opens this dialog
			// before closing the preferences dialog
			mailbox_->protocol (PROTOCOL_NONE);
			// If possible create a correct mailbox, otherwise a generic one
			// (to force lookup)
			if (!(mailbox = Mailbox::lookup_local (*mailbox_)))
				mailbox = new Mailbox (*mailbox_);
			preferences_->biff()->replace_mailbox (mailbox_, mailbox);
		}
	}

	// Third case: type is set to imap
	else if (selected_type_ == TYPE_IMAP) {
		// Protocol was not imap4 or mailbox was unknown, we change mailbox
		if ((mailbox_->protocol() != PROTOCOL_IMAP4)
			|| (mailbox_->status() == MAILBOX_UNKNOWN)) {
			Mailbox *mailbox = new Imap4 (*mailbox_);
			preferences_->biff()->replace_mailbox (mailbox_, mailbox);
		}
	}

	// Fourth case: type is set to pop
	else if (selected_type_ == TYPE_POP) {
		if (((mailbox_->protocol() != PROTOCOL_APOP)
			 || (mailbox_->status() == MAILBOX_UNKNOWN))
			&& (selected_auth_ == AUTH_APOP)) {
			Mailbox *mailbox = new Apop (*mailbox_);
			preferences_->biff()->replace_mailbox (mailbox_, mailbox);
		}
		else if ((mailbox_->protocol() != PROTOCOL_POP3)
				 || (mailbox_->status() == MAILBOX_UNKNOWN)) {
			Mailbox *mailbox = new Pop3 (*mailbox_);
			preferences_->biff()->replace_mailbox (mailbox_, mailbox);
		}
	}
	preferences_->synchronize ();
}

void
Properties::on_cancel (GtkWidget *widget)
{
	hide ();
	Preferences *prefs = preferences_;
	if (prefs->added()) {
		prefs->biff()->remove_mailbox (prefs->biff()->get(prefs->added()->uin()));
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

	// Insert the values of the options into the GUI widgets and update
	// widget status
	mailbox_->update_gui (OPTSGUI_UPDATE, OPTGRP_MAILBOX, xml_, filename_);

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
	auth_view (false);
	certificate_view (false);
	mailbox_view (false);
	delay_view (false);

	guint protocol = mailbox_->protocol ();
	selected_type_ = TYPE_AUTODETECT;

	if ((protocol == PROTOCOL_FILE) || (protocol == PROTOCOL_MH)
		|| (protocol == PROTOCOL_MH_BASIC)
		|| (protocol == PROTOCOL_MH_SYLPHEED)
		|| (protocol == PROTOCOL_MAILDIR)) {
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
		delay_view (true);
		mailbox_view (true);
		auth_view (true);
	}

	connection_view (true);

    gtk_combo_box_set_active (type_cbox_ ,selected_type_);
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
		if (!gtk_widget_get_visible (get("details_expander"))) {
			gtk_widget_show (get("details_expander"));
			gtk_size_group_add_widget (group_, get ("connection"));
			gtk_size_group_add_widget (group_, get ("authentication"));
			gtk_size_group_add_widget (group_, get ("certificate"));
			gtk_size_group_add_widget (group_, get ("delay"));
			gtk_size_group_add_widget (group_, get ("mailbox"));
		}
	}
	else {
		if (gtk_widget_get_visible (get("details_expander"))) {
			gtk_widget_hide (get("details_expander"));
			gtk_size_group_remove_widget (group_, get ("connection"));
			gtk_size_group_remove_widget (group_, get ("authentication"));
			gtk_size_group_remove_widget (group_, get ("certificate"));
			gtk_size_group_remove_widget (group_, get ("delay"));
			gtk_size_group_remove_widget (group_, get ("mailbox"));
		}
	}
}

/**
 *  Show or hide the widgets for setting the connection port. This function
 *  also sets the label for the standard port of the current selection.
 *
 *  Note: If calling several Properties::..._view(...) functions this one must
 *  be called after Properties::auth_view(...)!
 *
 *  @param visible  Whether the widgets shall be visible
 */
void 
Properties::connection_view (gboolean visible)
{
	// Show or hide the widgets
	if (visible)
		gtk_widget_show (get ("connection_alignment"));
	else
		gtk_widget_hide (get ("connection_alignment"));
	// Set the standard port label
	std::stringstream ss;
	ss << "(" << Mailbox::standard_port (selected_type_, selected_auth_, false)
	   << ")";
	gtk_label_set_text (GTK_LABEL (get ("label_standard_port")),
						ss.str().c_str());
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
	guint auth = AUTH_AUTODETECT;

	if (selected_auth_ == AUTH_NONE)
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
		if (selected_type_ == TYPE_POP)
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
	certificate_view (auth == AUTH_CERTIFICATE || auth == AUTH_TLS);
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
