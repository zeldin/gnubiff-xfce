// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2007 Nicolas Rougier, 2004-2007 Robert Sowada
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

const guint	MODE_GTK			=	0;
const guint	MODE_GNOME			=	1;
const guint MODE_NOGUI			=   2;
const guint MODE_SYSTEMTRAY		=   3;

const guint	PROTOCOL_NONE		=	0;
const guint	PROTOCOL_FILE		=	1;
const guint	PROTOCOL_POP3		=	2;
const guint	PROTOCOL_IMAP4		=	3;
const guint	PROTOCOL_MAILDIR	=	4;
const guint	PROTOCOL_MH			=	5;
const guint	PROTOCOL_APOP		=	6;
const guint	PROTOCOL_MH_BASIC	=	7;
const guint	PROTOCOL_MH_SYLPHEED=	8;

const guint	AUTH_AUTODETECT		=	0;
const guint	AUTH_USER_PASS		=	1;
const guint	AUTH_APOP			=	2;
const guint	AUTH_SSL			=	3;
const guint	AUTH_CERTIFICATE	=	4;
const guint	AUTH_TLS		=	5;
const guint	AUTH_NONE			=	(guint)-1;

const guint	MAILBOX_ERROR		=	0;
const guint	MAILBOX_EMPTY		=	1;
const guint	MAILBOX_OLD			=	2;
const guint	MAILBOX_NEW			=	3;
const guint	MAILBOX_CHECK		=	4;
const guint	MAILBOX_STOP		=	5;
const guint	MAILBOX_UNKNOWN		=	6;

const guint SIGNAL_NONE					=	0;
const guint SIGNAL_MARK_AS_READ			=	1;
const guint SIGNAL_START				=	2;
const guint SIGNAL_STOP					=	3;
const guint SIGNAL_POPUP_ENABLE			=	4;
const guint SIGNAL_POPUP_DISABLE		=	5;
const guint SIGNAL_POPUP_TOGGLE			=	6;
const guint SIGNAL_POPUP_SHOW			=	7;
const guint SIGNAL_POPUP_HIDE			=	8;
const guint SIGNAL_POPUP_TOGGLEVISIBLE	=	9;
const guint SIGNAL_STATUS_TO_STDOUT		=	10;

const guint OPTGRP_GENERAL		=	1;
const guint OPTGRP_APPLET		=	2;
const guint OPTGRP_POPUP		=	4;
const guint OPTGRP_MAILBOX		=	8;
const guint OPTGRP_INFORMATION	=	16;
const guint OPTGRP_SECURITY		=	32;

const guint LABEL_POS_LEFT_OUT    = 1;
const guint LABEL_POS_LEFT_IN     = 2;
const guint LABEL_POS_CENTER      = 3;
const guint LABEL_POS_RIGHT_IN    = 4;
const guint LABEL_POS_RIGHT_OUT   = 5;
const guint LABEL_POS_TOP_OUT     = 1;
const guint LABEL_POS_TOP_IN      = 2;
const guint LABEL_POS_BOT_IN      = 4;
const guint LABEL_POS_BOT_OUT     = 5;
const guint LABEL_POS_MANUALLY    = 1;
const guint LABEL_POS_LEFT_TOP    = 2;
const guint LABEL_POS_LEFT_BOT    = 3;
const guint LABEL_POS_RIGHT_TOP   = 4;
const guint LABEL_POS_RIGHT_BOT   = 5;

class Gnubiff_Options : public Options {
public:
protected:
	void add_options (guint groups, gboolean deprecated = false);

private:
	void add_options_applet (gboolean deprecated);
	void add_options_general (gboolean deprecated);
	void add_options_information (gboolean deprecated);
	void add_options_mailbox (gboolean deprecated);
	void add_options_popup (gboolean deprecated);
	void add_options_security (gboolean deprecated);

	const static guint protocol_int[];
	const static gchar *protocol_gchar[];
};

#endif
