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
// Author(s)     : Nicolas Rougier
// Short         : 
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#include "support.h"

#include <gtk/gtk.h>
#include <popt.h>

#ifdef USE_GNOME
#  include <gnome.h>
#  include <panel-applet.h>
#endif

#include "biff.h"
#include "nls.h"
#include "ui-preferences.h"
#include "ui-applet.h"
#include "ui-applet-gnome.h"
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

#ifdef USE_GNOME
	// Test for presence of "--oaf-ior-fd" option to determine if gnubiff is
	// started in gnome panel mode
 	for (gint i=0; i<argc; i++)
		if (std::string(argv[i]).find("--oaf-ior-fd")==0)
			return mainGNOME (argc, argv);
#endif

	//
	// Parse Options
	//
	poptContext poptcon;
	guint ui_mode = MODE_GTK;
	int status;
	char *config_file = 0;
	int no_configure = false, print_version = false, no_gui = false;
#if defined DEBUG && defined USE_GNOME
	int debug_applet=false;

   	static struct poptOption options_debug[] =
	{
	   	{"applet",'\0', POPT_ARG_NONE,   &debug_applet,  0,
		 N_("Start gnome applet from command line"), NULL},
		POPT_TABLEEND
	};
#endif
	static struct poptOption options_general[] =
	{
	   	{"config",      'c' , POPT_ARG_STRING, &config_file,   0,
		 N_("Configuration file to use"),  N_("file")},
		{"noconfigure", 'n' , POPT_ARG_NONE,   &no_configure,  0,
		 N_("Skip the configuration process"), NULL},
		{"nogui",		'\0', POPT_ARG_NONE,   &no_gui,		   0,
		 N_("Start gnubiff without GUI"), NULL},
		{"version",     'v' , POPT_ARG_NONE,   &print_version, 0,
		 N_("Print version information and exit"), NULL},
		POPT_TABLEEND
	};
	static struct poptOption options[] =
	{
	   	{NULL, '\0', POPT_ARG_INCLUDE_TABLE, &options_general, 0,
		 N_("General command line options:"), NULL },
#if defined DEBUG && defined USE_GNOME
		{NULL, '\0', POPT_ARG_INCLUDE_TABLE, &options_debug, 0,
		 N_("Options for debugging:"), NULL },
#endif
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	// Parse command line
	poptcon = poptGetContext ("gnubiff", argc,  (const char **) argv,
							  options, 0);
	while ((status = poptGetNextOpt(poptcon)) >= 0);
	if (status < -1) {
		fprintf (stderr, "%s: %s\n\n",
				 poptBadOption (poptcon, POPT_BADOPTION_NOALIAS),
				 poptStrerror (status));
		poptPrintHelp (poptcon, stderr, 0);
		exit (1);
	}
	poptGetNextOpt(poptcon);

	// Decide which frontend to use
	if (no_gui) {
		ui_mode = MODE_NOGUI;
		no_configure = true;
	}

#if defined DEBUG && defined USE_GNOME
	if (debug_applet)
		return mainGNOME (argc,argv);
#endif
	
	// GTK initialization
	gtk_init (&argc, &argv);

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

	// Start applet
	biff->applet()->start (!no_configure);

	// GTK main loop
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	// Exit
	return 0;
}

#ifdef USE_GNOME
static gboolean gnubiff_applet_factory (PanelApplet *applet, const gchar *iid,
										gpointer data)
{
	if (!strcmp (iid, "OAFIID:GNOME_gnubiffApplet")) {
		Biff *biff = new Biff (MODE_GNOME);
		AppletGnome *biffapplet = (AppletGnome *)biff->applet();
		biffapplet->dock ((GtkWidget *) applet);
		biffapplet->start (false);
	}
	return true;
}

int mainGNOME (int argc, char **argv) {
#if defined(PREFIX) && defined(SYSCONFDIR) && defined(DATADIR) && defined(LIBDIR)
	gnome_program_init ("gnubiff", "0", LIBGNOMEUI_MODULE, argc, argv,
						GNOME_PROGRAM_STANDARD_PROPERTIES, NULL);
#else
	gnome_program_init ("gnubiff", "0", LIBGNOMEUI_MODULE, argc, argv,
						GNOME_PARAM_NONE);
#endif

	panel_applet_factory_main ("OAFIID:GNOME_gnubiffApplet_Factory",
							   PANEL_TYPE_APPLET, gnubiff_applet_factory, 0);

	return 0;
}
#endif
