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

#ifndef __UI_CERTIFICATE_H__
#define __UI_CERTIFICATE_H__

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#ifdef HAVE_LIBSSL
#  include <openssl/ssl.h>
#  include <openssl/err.h>
#endif

#include "gui.h"

#define CERTIFICATE(x)	((Certificate *)(x))


class Certificate : public GUI {

protected:
#ifdef HAVE_LIBSSL
	X509 *			certificate_;
	X509 *			stored_certificate_;
#endif
	class Socket *	socket_;

public:
	/* base */
	Certificate (void);
	~Certificate (void);

	/* main */
	void select (class Socket *socket);
	void show (std::string name = "dialog");

	/* callbacks */
	void on_ok			(GtkWidget *widget);
	void on_cancel		(GtkWidget *widget);
	gboolean on_destroy (GtkWidget *widget,
						 GdkEvent *event);
	gboolean on_delete	(GtkWidget *widget,
						 GdkEvent *event);	
};

#endif
