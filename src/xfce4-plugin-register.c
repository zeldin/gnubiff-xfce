/* ========================================================================
** gnubiff -- a mail notification program
** Copyright (c) 2000-2011 Nicolas Rougier, 2004-2011 Robert Sowada
**
** This program is free software: you can redistribute it and/or
** modify it under the terms of the GNU General Public License as
** published by the Free Software Foundation, either version 3 of the
** License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
** ========================================================================
**
** File          : $RCSfile$
** Revision      : $Revision$
** Revision date : $Date$
** Author(s)     : Nicolas Rougier, Robert Sowada
** Short         : 
**
** This file is part of gnubiff.
**
** -*- mode:C; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
** ========================================================================
*/

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>

extern void APPLET_XFCE4_gnubiff_applet_construct (XfcePanelPlugin *plugin);
extern gboolean APPLET_XFCE4_gnubiff_applet_preinit (gint argc, gchar **argv);

/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER(APPLET_XFCE4_gnubiff_applet_construct);

XFCE_PANEL_DEFINE_PREINIT_FUNC(APPLET_XFCE4_gnubiff_applet_preinit);
