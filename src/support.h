// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2005 Nicolas Rougier
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
// Author(s)     : Nicolas Rougier, Robert Sowada
// Short         : Support functions
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#ifndef __SUPPORT_H__
#define __SUPPORT_H__

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif
#include "nls.h"


#include <glib.h>
#include <string>
#include <vector>

/**
 * This class provides various support functions.
 *
 * @see In the class Decoding are support functions for decoding, encoding
 *      and converting.
 */
class Support
{
protected:
	// Basic string functions
	gchar *utf8_strndup (const gchar *str, gsize n);

	// Advanced string functions
	std::string substitute (std::string format, std::string chars,
							std::vector<std::string> toinsert);
	gboolean numbersequence_to_vector (const std::string &seq,
									   std::vector<guint> &vec,
									   gboolean empty = true, char sep = ',',
									   char range = '-');

public:
	// Debugging
	static void unknown_internal_error_ (const gchar *file, guint line,
										 const gchar *func);
};

/**
 *  Print debug information. This function should be called in a situation
 *  that should never happen to provide more information in a bug report.
 */
#define unknown_internal_error() (Support::unknown_internal_error_ (__FILE__, __LINE__, __func__))

#endif
