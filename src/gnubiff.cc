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

#include "support.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <stdlib.h>

#ifdef USE_GNOME
#	include <panel-applet.h>
#	include "ui-applet-gnome.h"
#endif

#include "biff.h"
#include "nls.h"
#include "ui-preferences.h"
#include "ui-applet.h"
#include "ui-applet-gtk.h"

int main (int argc, char **argv);
int mainGNOME (int argc, char **argv);


int main (int argc, char **argv) {
	// Initialize i18n support
	setlocale (LC_ALL, "");
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, GNUBIFF_LOCALEDIR);
#   ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#   endif
	textdomain (GETTEXT_PACKAGE);
#endif

	// Thread initialization
	g_thread_init (NULL);
	gdk_threads_init ();

	//
	// Parse Options
	//
	GMainLoop	*gmainloop = NULL;
	GError		*gerror = NULL;
	guint		ui_mode = MODE_GTK;
	char		*config_file = 0;
	int			no_configure = false, print_version = false, no_gui = false;
	int			systemtray = false;
#if defined USE_GNOME
	int			debug_applet=false;

	static GOptionEntry options_debug[] =
	{
		{"applet",'\0', 0, G_OPTION_ARG_NONE, &debug_applet,
		 N_("Start gnome applet from command line"), NULL},
        {NULL}
	};
#endif
	static GOptionEntry options_general[] =
	{
		{"config", 'c', 0, G_OPTION_ARG_FILENAME, &config_file,
		 N_("Configuration file to use"), N_("file")},
		{"noconfigure", 'n', 0, G_OPTION_ARG_NONE, &no_configure,
		 N_("Skip the configuration process"), NULL},
		{"nogui", '\0', 0, G_OPTION_ARG_NONE, &no_gui,
		 N_("Start gnubiff without GUI"), NULL},
		{"systemtray", '\0', 0, G_OPTION_ARG_NONE, &systemtray,
		 N_("Put gnubiff's icon into the system tray"), NULL},
		{"version", 'v', 0, G_OPTION_ARG_NONE, &print_version,
		 N_("Print version information and exit"), NULL},
        {NULL}
	};

	// Parse command line
	GOptionContext *optcon = g_option_context_new ("");
	g_option_context_add_main_entries (optcon, options_general,
									   GETTEXT_PACKAGE);
#if defined USE_GNOME
	GOptionGroup *optgrp_dbg;
	optgrp_dbg = g_option_group_new ("debug", _("Options for debugging:"),
									 _("Show debugging options"), NULL, NULL);
	g_option_group_add_entries (optgrp_dbg, options_debug);
	g_option_context_add_group (optcon, optgrp_dbg);
#endif
	g_option_context_add_group (optcon, gtk_get_option_group (true));
	if (!g_option_context_parse (optcon, &argc, &argv, &gerror)) {
		g_warning (_("Cannot parse command line options: %s"),gerror->message);
		exit (EXIT_FAILURE);
	}

	// Decide which frontend to use
	if (no_gui) {
		ui_mode = MODE_NOGUI;
		no_configure = true;
	}
	if (systemtray)
		ui_mode = MODE_SYSTEMTRAY;

	// Initialization of Glib and (if needed) GTK
	if (no_gui)
		gmainloop = g_main_loop_new (NULL, true);
	else
		gtk_init (&argc, &argv);

#if defined USE_GNOME
	if (debug_applet)
		return mainGNOME (argc, argv);
#endif

	// Print version information if requested and exit
	if (print_version) {
#ifdef IS_CVS_VERSION
		g_print ("gnubiff version "PACKAGE_VERSION" CVS\n");
#else
		g_print ("gnubiff version "PACKAGE_VERSION"\n");
#endif
		exit (EXIT_SUCCESS);
	}

	// Create biff with configuration file (or not)
	Biff *biff;
	if (config_file)
		biff = new Biff (ui_mode, config_file);
	else
		biff = new Biff (ui_mode);

	// Decide whether the preferences dialog shall be shown now
	no_configure = no_configure || !biff->value_bool ("startup_preferences");

	// Start applet
	biff->applet()->start (!no_configure);

	// Main loop
	if (no_gui)
		g_main_loop_run (gmainloop);
	else {
		gdk_threads_enter();
		gtk_main();
		gdk_threads_leave();
	}

	// Exit
	return 0;
}

#ifdef USE_GNOME

int mainGNOME (int argc, char **argv) {
	panel_applet_factory_main ("GnubiffApplet_Factory",
							   PANEL_TYPE_APPLET,
							   AppletGnome::gnubiff_applet_factory, 0);

	return 0;
}
#endif
