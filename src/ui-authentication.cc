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
#include "ui-authentication.h"
#include "mailbox.h"


Authentication::Authentication (void) : GUI (GNUBIFF_DATADIR"/authentication.glade")
{
	mailbox_ = 0;
	access_mutex_ = g_mutex_new ();
}

Authentication::~Authentication (void)
{
	GtkWidget *widget = get("dialog");
	if (GTK_IS_WIDGET (widget)) {
		hide ();
		gtk_widget_destroy (widget);
	}
	if (GTK_IS_OBJECT (xml_))
		g_object_unref (xml_);
}

void
Authentication::select (Mailbox *mailbox)
{
	if (mailbox) {
		g_mutex_lock (access_mutex_);
		mailbox_ = mailbox;
		gdk_threads_enter();
		show ();
		gdk_threads_leave();
	}
}

void
Authentication::show (std::string name)
{
	if (!xml_)
		create();

	// Try to identify mailbox by:
	// 1. using name
	std::string id = mailbox_->name();

	// 2. using hostname
	if (id.empty())
		id = mailbox_->hostname();

	// 3. using mailbox uin
	if (id.empty()) {
		std::stringstream s;
		s << mailbox_->uin();
		id = s.str();
	}

	gchar *text = g_strdup_printf (_("Please enter your username and password for mailbox '%s'"), id.c_str());
	gtk_label_set_text (GTK_LABEL(get("label")), text);
	g_free (text);
	gtk_entry_set_text (GTK_ENTRY (get("username_entry")), mailbox_->username().c_str());
	gtk_entry_set_text (GTK_ENTRY (get("password_entry")), mailbox_->password().c_str());
	gtk_widget_show_all (get("dialog"));
	gtk_main ();
}

void
Authentication::on_ok (GtkWidget *widget)
{
	gtk_main_quit();
	mailbox_->username (gtk_entry_get_text (GTK_ENTRY (get("username_entry"))));
	mailbox_->password (gtk_entry_get_text (GTK_ENTRY (get("password_entry"))));
	hide();
	g_mutex_unlock (access_mutex_);
	gtk_main_quit ();
}

void
Authentication::on_cancel (GtkWidget *widget)
{
	hide();
	g_mutex_unlock (access_mutex_);
	gtk_main_quit ();
}

gboolean
Authentication::on_destroy (GtkWidget *widget,
							GdkEvent *event)
{
	hide();
	g_mutex_unlock (access_mutex_);
	gtk_main_quit ();
	return TRUE;
}

gboolean
Authentication::on_delete	(GtkWidget *widget,
							 GdkEvent *event)
{
	hide();
	g_mutex_unlock (access_mutex_);
	gtk_main_quit ();
	return TRUE;
}
