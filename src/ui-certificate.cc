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

#include "ui-certificate.h"
#include "socket.h"


Certificate::Certificate (void) : GUI (GNUBIFF_DATADIR"/certificate.glade")
{
#ifdef HAVE_LIBSSL
	socket_ = 0;
	certificate_ = 0;
#endif
	stored_certificate_ = 0;
}

Certificate::~Certificate (void)
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
Certificate::select (Socket *socket)
{
	if (socket) {
		socket_ = socket;
		show ();
	}
}

void
Certificate::show (std::string name)
{
	if (!xml_)
		create();

	gchar *text1 = g_strdup_printf (_("Unable to verify the identity of %s as a trusted site.\n"), socket_->hostname().c_str());
	gchar *text2 = g_strdup_printf (_("Either site's certificate is incomplete or you're connected to a site pretending to be %s, possibly to obtain your password"), socket_->hostname().c_str());
	std::string text = std::string(text1) + std::string(text2);
	gtk_label_set_text (GTK_LABEL(get("label")), text.c_str());
	g_free (text1);
	g_free (text2);

#ifdef HAVE_LIBSSL
	SSL *ssl = socket_->ssl();
	if (!ssl)
		return;

	certificate_ = SSL_get_peer_certificate (ssl);
	if (!certificate_)
		return;

	if ((stored_certificate_) && X509_cmp (stored_certificate_, certificate_)) {
		socket_->bypass_certificate (true);
		return;
	}

	char common_name[100];
	char country[100];
	char state[100];
	char locality[100];
	char org[100];
	char unit[100];
	common_name[0] = '\0';
	country[0] = '\0';
	state[0] = '\0';
	locality[0] = '\0';
	org[0] = '\0';
	unit[0] = '\0';
 
	X509_NAME_get_text_by_NID (X509_get_subject_name(certificate_), NID_commonName, common_name, 100);
	X509_NAME_get_text_by_NID (X509_get_subject_name(certificate_), NID_organizationName, org, 100);
	X509_NAME_get_text_by_NID (X509_get_subject_name(certificate_), NID_organizationalUnitName, unit, 100);
	X509_NAME_get_text_by_NID (X509_get_subject_name(certificate_), NID_countryName, country, 100);
	X509_NAME_get_text_by_NID (X509_get_subject_name(certificate_), NID_stateOrProvinceName, state, 100);
	X509_NAME_get_text_by_NID (X509_get_subject_name(certificate_), NID_localityName, locality, 100);
	gtk_label_set_text (GTK_LABEL(get("peer_CN")), common_name);
	gtk_label_set_text (GTK_LABEL(get("peer_O")), org);
	gtk_label_set_text (GTK_LABEL(get("peer_OU")), unit);
	gtk_label_set_text (GTK_LABEL(get("peer_CO")), country);
	gtk_label_set_text (GTK_LABEL(get("peer_S")), state);
	gtk_label_set_text (GTK_LABEL(get("peer_L")), locality);
	gtk_widget_show_all (get("dialog"));
	gtk_main ();
#endif
}

void
Certificate::on_ok (GtkWidget *widget)
{
#ifdef HAVE_LIBSSL
	stored_certificate_ = certificate_;
	certificate_ = 0;
#endif
	socket_->bypass_certificate (true);
	hide();
	gtk_main_quit();
}

void
Certificate::on_cancel (GtkWidget *widget)
{
#ifdef HAVE_LIBSSL
	stored_certificate_ = 0;
	certificate_ = 0;
#endif
	socket_->bypass_certificate (false);
	hide();
	gtk_main_quit();
}

gboolean
Certificate::on_destroy (GtkWidget *widget,
							GdkEvent *event)
{
#ifdef HAVE_LIBSSL
	stored_certificate_ = 0;
	certificate_ = 0;
#endif
	socket_->bypass_certificate (false);
	hide();
	gtk_main_quit();
	return TRUE;
}

gboolean
Certificate::on_delete	(GtkWidget *widget,
							 GdkEvent *event)
{
#ifdef HAVE_LIBSSL
	stored_certificate_ = 0;
	certificate_ = 0;
#endif
	socket_->bypass_certificate (false);
	hide();
	gtk_main_quit();
	return TRUE;
}
