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

#include <gmodule.h>
#include <stdlib.h>
#include "gtk_image_animation.h"
#include "gui.h"
#include "callbacks.h"


/**
 * Local variable for animation preview in gtk_file_chooser
 **/
GtkImageAnimation *preview_animation = NULL;


/**
 * "C" binding
 **/
extern "C" {
	gboolean GUI_on_delete_event (GtkWidget *widget,
								  GdkEvent *event,
								  gpointer data)
	{
		if (data)
			return (static_cast<GUI *>(data))->on_delete (widget, event);
		else
			unknown_internal_error ();
		return false;
	}

	gboolean GUI_on_destroy_event (GtkWidget *widget,
								   GdkEvent *event,
								   gpointer data)
	{
		if (data)
			return (static_cast<GUI *>(data))->on_destroy (widget, event);
		else
			unknown_internal_error ();
		return false;

	}

	void GUI_on_ok (GtkWidget *widget,
					gpointer data)
	{
		if (data)
			(static_cast<GUI *>(data))->on_ok (widget);
		else
			unknown_internal_error ();
	}

	void GUI_on_apply (GtkWidget *widget,
						 gpointer data)
	{
		if (data)
			(static_cast<GUI *>(data))->on_apply (widget);
		else
			unknown_internal_error ();
	}

	void GUI_on_close (GtkWidget *widget,
					   gpointer data)
	{
		if (data)
			(static_cast<GUI *>(data))->on_close (widget);
		else
			unknown_internal_error ();
	}

	void GUI_on_cancel (GtkWidget *widget,
						gpointer data)
	{
		if (data)
			(static_cast<GUI *>(data))->on_cancel (widget);
		else
			unknown_internal_error ();
	}

	/*
	 * From mail-notification applet
	 * Copyright (c) 2003, 2004 Jean-Yves Lefort.
	 *
	 * The GtkFileChooser API does not allow a chooser to pick a file
	 * (GTK_FILE_CHOOSER_ACTION_OPEN) and select a folder
	 * (GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) at the same time.
	 *
	 * This function provides a workaround.
	 */
	static void
	GUI_file_chooser_dialog_file_activated_h (GtkFileChooser *chooser,
											  gpointer user_data)
	{
		int accept_id = GPOINTER_TO_INT(user_data);
		gtk_dialog_response (GTK_DIALOG(chooser), accept_id);
	}

	static void
	GUI_file_chooser_dialog_response_h (GtkDialog *dialog,
										int response_id,
										gpointer user_data)
	{
		int accept_id = GPOINTER_TO_INT (user_data);
		if (response_id == accept_id) {
			char *uri;
			uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER(dialog));
			if (uri)
				g_free(uri);
			else
				g_signal_stop_emission_by_name (dialog, "response");
		}
	}

	void
	GUI_file_chooser_dialog_allow_select_folder (GtkFileChooserDialog *dialog,
												 int accept_id)
	{
		g_return_if_fail (GTK_IS_FILE_CHOOSER_DIALOG (dialog));
		g_return_if_fail (gtk_file_chooser_get_action(GTK_FILE_CHOOSER(dialog)) == GTK_FILE_CHOOSER_ACTION_OPEN);
		g_return_if_fail(! (accept_id == GTK_RESPONSE_ACCEPT
							|| accept_id == GTK_RESPONSE_OK
							|| accept_id == GTK_RESPONSE_YES
							|| accept_id == GTK_RESPONSE_APPLY));
		g_signal_connect(G_OBJECT(dialog),
						 "file-activated",
						 G_CALLBACK(GUI_file_chooser_dialog_file_activated_h),
						 GINT_TO_POINTER(accept_id));
		g_signal_connect(G_OBJECT(dialog),
						 "response",
						 G_CALLBACK(GUI_file_chooser_dialog_response_h),
						 GINT_TO_POINTER(accept_id));
	}

}


GUI::GUI (std::string filename)
{
	filename_   = filename;
	gtkbuilder_ = NULL;
}

GUI::~GUI (void)
{
	if (gtkbuilder_)
		g_object_unref (G_OBJECT(gtkbuilder_));
    gtkbuilder_ = NULL;
}

gint
GUI::create (gpointer callbackdata)
{
	if (gtkbuilder_)
		return true;

	GError* error = NULL;
	gtkbuilder_ = gtk_builder_new ();
	if (!gtk_builder_add_from_file (gtkbuilder_, filename_.c_str(), &error))
	{
		g_warning (_("Couldn't load builder file \"%s\": %s"), filename_.c_str(), error->message);
        g_error_free (error);
        exit (EXIT_FAILURE);
    }

        gtk_builder_add_callback_symbols (gtkbuilder_, ALL_CALLBACKS, NULL);
    gtk_builder_connect_signals (gtkbuilder_, callbackdata);
	create_insert_version ();
	return true;
}

/**
 *  Insert version and date information into certain widgets. For widgets
 *  called "gnubiff" (if existing) the following substitutions are made:
 *  "%v" is substituted by the version number, "%c" is substituted by "CVS" if
 *  it's a cvs version of gnubiff and "" otherwise.
 *
 *  The version information for the about dialog is also set if this dialog
 *  is present.
 */
void 
GUI::create_insert_version (void)
{
	static const gchar *widgets[] = {"gnubiff", NULL};
	static const std::string chars = "vc";
	std::vector<std::string> toinsert (2);
	toinsert[0] = std::string (PACKAGE_VERSION);
	toinsert[1] = std::string (IS_CVS_VERSION ? "CVS" : "");
	guint i = 0;
	while (widgets[i] != NULL) {
		// Don't use GUI::get() because we don't want warning messages if
		// the widget doesn't exist
		GtkLabel *label = GTK_LABEL (gtk_builder_get_object (gtkbuilder_, widgets[i++]));
		if (!label)
			continue;
		// Substitute
		const gchar *text = gtk_label_get_label (label);
		if (!text)
			continue;
		std::string newtext = substitute (text, chars, toinsert);
		gtk_label_set_label (label, newtext.c_str ());
	}

	// About dialog
	GtkAboutDialog *about;
	about = GTK_ABOUT_DIALOG (gtk_builder_get_object (gtkbuilder_, "gnubiffabout"));
	if (about) {
		std::string newtext = substitute ("%v %c", chars, toinsert);
		gtk_about_dialog_set_version (about, newtext.c_str ());
	}
}

void
GUI::show (std::string name)
{
	if (gtkbuilder_)
		gtk_widget_show (get(name));
}

void
GUI::hide (std::string name)
{
	if (gtkbuilder_)
		gtk_widget_hide (get (name));
}

GtkWidget *
GUI::get (std::string name)
{
	GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (gtkbuilder_, static_cast<const gchar *>(name.c_str())));
    
	if (!widget)
		g_warning (_("Cannot find the specified widget (\"%s\")"
					 " within xml structure (\"%s\")"), name.c_str(),
				   filename_.c_str());
	return widget;
}

gboolean
GUI::browse (std::string title,
			 std::string widget_name,
			 gboolean file_and_folder,
			 GtkWidget *preview)
{
	GtkWidget *chooser = 
		gtk_file_chooser_dialog_new (title.c_str(), 0,
									 GTK_FILE_CHOOSER_ACTION_OPEN,
									 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									 GTK_STOCK_OPEN, 1,
									 NULL);
	g_object_set (G_OBJECT(chooser), "show-hidden", true, NULL);
	if (file_and_folder)
		GUI_file_chooser_dialog_allow_select_folder(GTK_FILE_CHOOSER_DIALOG(chooser), FALSE);
	if (preview) {
		gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (chooser), preview);
		g_signal_connect (chooser, "update-preview", G_CALLBACK (GUI_update_preview), preview);
	}
	gtk_window_set_position (GTK_WINDOW (chooser), GTK_WIN_POS_CENTER);
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser),
								   gtk_entry_get_text (GTK_ENTRY (get(widget_name.c_str()))));
	gint result = gtk_dialog_run (GTK_DIALOG (chooser));
	
	if (result == 1) {
		char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
		gtk_entry_set_text (GTK_ENTRY (get(widget_name.c_str())), filename);
		g_free (filename);
	}
	if (preview && preview_animation)
		preview_animation->stop();
	gtk_widget_destroy (chooser);

	return result;
}

std::string
GUI::utf8_to_filename (std::string text)
{
  gchar *buffer = g_filename_from_utf8 (text.c_str(),-1,0,0,0);
  std::string newtext (buffer);
  g_free (buffer);
  return newtext;
}

std::string
GUI::utf8_to_locale (std::string text)
{
	gchar *buffer = g_locale_from_utf8 (text.c_str(),-1,0,0,0);
	std::string newtext (buffer);
	g_free (buffer);
	return newtext;
}

std::string
GUI::filename_to_utf8 (std::string text)
{
	gchar *buffer = g_filename_to_utf8 (text.c_str(),-1,0,0,0);
	std::string newtext (buffer);
	g_free (buffer);
	return newtext;
}

std::string
GUI::locale_to_utf8 (std::string text)
{
	gchar *buffer = g_locale_to_utf8 (text.c_str(),-1,0,0,0);
	std::string newtext (buffer);
	g_free (buffer);
	return newtext;
}

gboolean
GUI::on_delete (GtkWidget *widget,
				GdkEvent *event)
{
	gtk_widget_hide (widget);
	return true;
}

gboolean
GUI::on_destroy (GtkWidget *widget,
				 GdkEvent *event)
{
	gtk_main_quit();
	return true;
}

/*void
GUI_connect (const gchar *handler_name,
			 GObject *object,
			 const gchar *signal_name,
			 const gchar *signal_data,
			 GObject *connect_object,
			 gboolean after,
			 gpointer user_data)
{
	GModule *symbols = 0;
	GtkSignalFunc func = NULL, *func_ptr = &func;
 
	if (!symbols) {
		if (!g_module_supported()) {
			g_error (_("GUI_connect requires working gmodule"));
			exit (1);
		}
		symbols = g_module_open (0, G_MODULE_BIND_MASK);
	}

	if (!g_module_symbol(symbols, handler_name,(gpointer *) func_ptr))
		g_warning(_("Could not find signal handler '%s'."), handler_name);
	else
		g_signal_connect (object, signal_name, GTK_SIGNAL_FUNC(func), user_data);
}*/

void
GUI_update_preview (GtkWidget *widget,
					gpointer data)
{
	GtkWidget *preview;
	char *filename;
	gboolean have_preview = false;
	preview = GTK_WIDGET (data);
	if (preview_animation == 0)
		preview_animation = new GtkImageAnimation (GTK_IMAGE (preview));
	preview_animation->attach (GTK_IMAGE (preview));
	filename = gtk_file_chooser_get_preview_filename (GTK_FILE_CHOOSER(widget));
	if (filename) {
		have_preview = 	preview_animation->open (filename);
		g_free (filename);	
	}
	else
		have_preview = false;
	if (have_preview)
		preview_animation->start();
	gtk_file_chooser_set_preview_widget_active (GTK_FILE_CHOOSER(widget), have_preview);
}
