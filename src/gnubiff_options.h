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
// Author(s)     : Robert Sowada, Nicolas Rougier
// Short         : Options for gnubiff
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#ifndef __GNUBIFF_OPTIONS_H__
#define __GNUBIFF_OPTIONS_H__

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif
#include <glib.h>
#include "options.h"

/**
 * Constant definitions
 **/

const guint	MANUAL_CHECK		=	0;
const guint	AUTOMATIC_CHECK		=	1;

const guint	GTK_MODE			=	0;
const guint	GNOME_MODE			=	1;

const guint	PROTOCOL_NONE		=	0;
const guint	PROTOCOL_FILE		=	1;
const guint	PROTOCOL_POP3		=	2;
const guint	PROTOCOL_IMAP4		=	3;
const guint	PROTOCOL_MAILDIR	=	4;
const guint	PROTOCOL_MH			=	5;
const guint	PROTOCOL_APOP		=	6;

const guint	AUTH_AUTODETECT		=	0;
const guint	AUTH_USER_PASS		=	1;
const guint	AUTH_APOP			=	2;
const guint	AUTH_SSL			=	3;
const guint	AUTH_CERTIFICATE	=	4;
const guint	AUTH_NONE			=	(guint)-1;

const guint	MAILBOX_ERROR		=	0;
const guint	MAILBOX_EMPTY		=	1;
const guint	MAILBOX_OLD			=	2;
const guint	MAILBOX_NEW			=	3;
const guint	MAILBOX_CHECK		=	4;
const guint	MAILBOX_STOP		=	5;
const guint	MAILBOX_UNKNOWN		=	6;

const guint OPTGRP_GENERAL		=	1;
const guint OPTGRP_APPLET		=	2;
const guint OPTGRP_POPUP		=	4;
const guint OPTGRP_MAILBOX		=	8;
const guint OPTGRP_INFORMATION	=	16;
const guint OPTGRP_SECURITY		=	32;
const guint OPTGRP_MAILS		=	64;

class Gnubiff_Options : public Options {
public:
protected:
	void add_options (guint groups);
	void add_options_applet (void);
	void add_options_general (void);
	void add_options_information (void);
	void add_options_mailbox (void);
	void add_options_popup (void);
private:
	const static guint protocol_int[];
	const static gchar *protocol_gchar[];
};

#endif
