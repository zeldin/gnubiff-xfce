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

#ifndef __UI_PROPERTIES_H__
#define __UI_PROPERTIES_H__

#include "gnubiff_options.h"
#include "gui.h"

// Use of corresponding constants allows us to use functions like
// Mailbox::standard_port()
const gint	TYPE_AUTODETECT			=	PROTOCOL_NONE;
const gint	TYPE_LOCAL				=	PROTOCOL_FILE;
const gint	TYPE_POP				=	PROTOCOL_POP3;
const gint	TYPE_IMAP				=	PROTOCOL_IMAP4;

#define PROPERTIES(x)	(static_cast<Properties *>(x))


class Properties : public GUI {

protected:
	class Preferences *	preferences_;
	class Mailbox	*	mailbox_;
    GtkComboBoxText     *auth_cbox_;
    GtkComboBoxText     *type_cbox_;
	GtkSizeGroup *		group_;

	gint				selected_type_;
	guint				selected_auth_;

public:
	// ========================================================================
	//  Base
	// ========================================================================
	Properties (class Preferences *preferences);
	~Properties (void);

	/* main */
	gboolean create (gpointer callbackdata);
	void show (std::string name = "dialog");

	// ========================================================================
	//  Access
	// ========================================================================
	void select (class Mailbox *mailbox);
	class Mailbox *mailbox (void)			{return mailbox_;}

	// ========================================================================
	//  GUI modification
	// ========================================================================
	void update_view 			(void);
	void type_view				(void);
	void set_type 				(gint type);
	void identity_view			(gboolean visible);
	void details_view			(gboolean visible);
	void connection_view		(gboolean visible);
	void auth_view				(gboolean visible);
	void certificate_view		(gboolean visible);
	void mailbox_view			(gboolean visible);
	void delay_view				(gboolean visible);

	// ========================================================================
	//  Callbacks
	// ========================================================================
	void on_delay					(GtkWidget *widget);
	void on_port					(GtkWidget *widget);
	void on_mailbox					(GtkWidget *widget);
	void on_type_changed			(void);
	void on_auth_changed			(void);
	void on_browse_address			(GtkWidget *widget);
	void on_browse_certificate		(GtkWidget *widget);
	void on_ok						(GtkWidget *widget);
	void on_apply					(GtkWidget *widget);
	void on_cancel					(GtkWidget *widget);
	gboolean on_destroy 			(GtkWidget *widget,
									 GdkEvent *event);
	gboolean on_delete				(GtkWidget *widget,
									 GdkEvent *event);	
};

#endif
