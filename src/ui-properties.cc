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

#include <sstream>
#include "nls.h"
#include "ui-preferences.h"
#include "ui-properties.h"
#include "mailbox.h"


/**
 * "C" binding
 **/
extern "C" {
	void PROPERTIES_on_browse_location (GtkWidget *widget,
										gpointer data)
	{
		PROPERTIES(data)->on_browse_location (widget);
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
#ifndef HAVE_LIBSSL
	gtk_widget_set_sensitive (get("ssl_check"), FALSE);
	gtk_widget_set_sensitive (get("certificate_entry"), FALSE);
	gtk_widget_set_sensitive (get("browse_certificate"), FALSE);
	gtk_widget_set_sensitive (get("certificate"), FALSE);
#endif
	return result;
}

void
Properties::select (Mailbox *mailbox)
{
	mailbox_ = mailbox;
	if (!mailbox) {
		hide();
		return;
	}
	gtk_entry_set_text (GTK_ENTRY (get("name_entry")), mailbox_->name().c_str());
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("poll_spin")), mailbox_->polltime());
	gtk_entry_set_text (GTK_ENTRY (get("location_entry")), mailbox_->location().c_str());
	gtk_entry_set_text (GTK_ENTRY (get("hostname_entry")), mailbox_->hostname().c_str());
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(get("port_spin")), mailbox_->port());
	gtk_entry_set_text (GTK_ENTRY (get("username_entry")), mailbox_->username().c_str());
	gtk_entry_set_text (GTK_ENTRY (get("password_entry")), mailbox_->password().c_str());
	gtk_entry_set_text (GTK_ENTRY (get("folder_entry")), mailbox_->folder().c_str());
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("ssl_check")), mailbox_->use_ssl());
	gtk_entry_set_text (GTK_ENTRY (get("certificate_entry")), mailbox_->certificate().c_str());
	if (mailbox_->is_local ()) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("file_radio")), true);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("network_radio")), false);
	}
	else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("file_radio")), false);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(get("network_radio")), true);
	}
}

void
Properties::show (std::string name)
{
	if (mailbox_) {
		select (mailbox_);
		gtk_widget_show_all (get("dialog"));
	}
}

void
Properties::on_ok (GtkWidget *widget)
{
	on_apply (widget);
	preferences_->added (0);
	hide ();
}

void
Properties::on_apply (GtkWidget *widget)
{
	if (mailbox_) {
		mailbox_->name (gtk_entry_get_text (GTK_ENTRY (get("name_entry"))));
		mailbox_->polltime ((guint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(get("poll_spin"))));
		mailbox_->location (gtk_entry_get_text (GTK_ENTRY (get("location_entry"))));
		mailbox_->hostname (gtk_entry_get_text (GTK_ENTRY (get("hostname_entry"))));
		mailbox_->port ((guint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(get("port_spin"))));
		mailbox_->username (gtk_entry_get_text (GTK_ENTRY (get("username_entry"))));
		mailbox_->password (gtk_entry_get_text (GTK_ENTRY (get("password_entry"))));
		mailbox_->folder (gtk_entry_get_text (GTK_ENTRY (get("folder_entry"))));
		mailbox_->use_ssl (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("ssl_check"))));
		mailbox_->certificate (gtk_entry_get_text (GTK_ENTRY (get("certificate_entry"))));
		if ((mailbox_->is_local() && !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("file_radio")))) ||
			(!mailbox_->is_local() && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("file_radio"))))) {
			mailbox_->is_local (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(get("file_radio"))));
			preferences_->biff()->lookup();
		}
		preferences_->synchronize();
	}
}

void
Properties::on_cancel (GtkWidget *widget)
{
	hide ();
	Preferences *prefs = preferences_;
	if (prefs->added()) {
		prefs->biff()->remove (prefs->added()->uin());
		prefs->added(0);
		prefs->synchronize ();		
	}

}

void
Properties::on_browse_location (GtkWidget *widget)
{
	browse (_("Browse for a location"), "location_entry", true);
}

void
Properties::on_browse_certificate (GtkWidget *widget)
{
	browse (_("Browse for a certificate"), "certificate_entry");
}

gboolean
Properties::on_destroy (GtkWidget *widget,
						GdkEvent *event)
{
	on_cancel(0);
	return TRUE;
}

gboolean
Properties::on_delete	(GtkWidget *widget,
						 GdkEvent *event)
{
	on_cancel(0);
	return TRUE;
}
