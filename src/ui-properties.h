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

#define PROPERTIES(x)	((Properties *)(x))


class Properties : public GUI {

protected:
	class Preferences *	preferences_;
	class Mailbox	*	mailbox_;

public:
	/* base */
	Properties (class Preferences *preferences);
	~Properties (void);

	/* main */
	gboolean create (void);
	void show (std::string name = "dialog");

	/* access */
	void select (class Mailbox *mailbox);
	class Mailbox *mailbox (void)			{return mailbox_;}

	/* callbacks */
	void on_ok					(GtkWidget *widget);
	void on_apply				(GtkWidget *widget);
	void on_cancel				(GtkWidget *widget);
	void on_browse_location		(GtkWidget *widget);
	void on_browse_certificate	(GtkWidget *widget);
	gboolean on_destroy 		(GtkWidget *widget,
								 GdkEvent *event);
	gboolean on_delete			(GtkWidget *widget,
								 GdkEvent *event);	
};

#endif
