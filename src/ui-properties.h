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

#ifndef __UI_PROPERTIES_H__
#define __UI_PROPERTIES_H__

#include "gui.h"

const gint	TYPE_AUTODETECT			=	0;
const gint	TYPE_LOCAL				=	1;
const gint	TYPE_POP				=	2;
const gint	TYPE_IMAP				=	3;

#define PROPERTIES(x)	((Properties *)(x))


class Properties : public GUI {

protected:
	class Preferences *	preferences_;
	class Mailbox	*	mailbox_;
	GtkUIManager *		type_manager_;
	GtkWidget *			type_menu_;
	GtkUIManager *		auth_manager_;
	GtkWidget *			auth_menu_;
	GtkSizeGroup *		group_;

	gint				selected_type_;
	gint				selected_auth_;

public:
	// ========================================================================
	//  Base
	// ========================================================================
	Properties (class Preferences *preferences);
	~Properties (void);

	/* main */
	gboolean create (void);
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
	void on_type_changed			(GtkAction *action);
	void on_auth_changed			(GtkAction *action);
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
