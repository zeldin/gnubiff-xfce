/* gnubiff -- a mail notification program
 * Copyright (c) 2000-2004 Nicolas Rougier
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * This file is part of gnubiff.
 */
#include <glade/glade.h>
#include <iostream>
#include <gmodule.h>
#include "Biff.h"
#include "GUI.h"



// ===================================================================
// = Callbacks "C" binding ===========================================
// ===================================================================
extern "C" {
  //! Automatic signal connection for libglade, using gmodule
  void GUI_connect (const gchar *handler_name,
		    GObject *object,
		    const gchar *signal_name,
		    const gchar *signal_data,
		    GObject *connect_object,
		    gboolean after,
		    gpointer user_data);
  //! C binding
  gboolean GUI_on_delete (GtkWidget *widget, GdkEvent *event, gpointer data)	{return ((GUI *)data)->on_delete (widget, event);}
  //! C binding
  gboolean GUI_on_destroy (GtkWidget *widget, GdkEvent *event, gpointer data)	{return ((GUI *)data)->on_destroy (widget, event);}
}


// ===================================================================
// = GUI =============================================================
/**
   This method creates and initialize the object but do not load the
   interface
   \param xmlFilename is the name of the glade file to use
*/
// ===================================================================
GUI::GUI (std::string xmlFilename) {
  _xmlFilename = xmlFilename;
  _xml = 0;
}


// ===================================================================
// = ~GUI ============================================================
/**
   This method destroys the interface taking care of memory
 */
// ===================================================================
GUI::~GUI (void) {
  if (_xml)
    g_object_unref (G_OBJECT(_xml));
}


// ===================================================================
// = create ==========================================================
/**
   Creation of the interface by calling the glade_xml_new function
   @return true
*/
// ===================================================================
gint GUI::create (void) {
  // If interface has already been created, simply return
  if (_xml)
    return true;

  // Try to load the glade file and build the interface
  _xml = glade_xml_new (_xmlFilename.c_str(), NULL, NULL);

  // Check if all's OK
  if (!_xml) {
    // Here we've a problem, either interface file was not found or
    // there's a real problem with construction.
    // First step: a warning dialog
    gchar *basename = g_path_get_basename (_xmlFilename.c_str());
    gchar *path = g_path_get_dirname (_xmlFilename.c_str());
    GtkWidget *dialog =
      gtk_message_dialog_new (NULL,
			      GTK_DIALOG_MODAL,
			      GTK_MESSAGE_ERROR,
			      GTK_BUTTONS_OK,
			      _("Cannot build the interface.\n\n" \
			      "Name: %s\nPath: %s\n\n"\
			      "Please make sure package has been installed correctly."),
			      basename, path);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    g_free (basename);
    g_free (path);
    
    // This is a fatal error, we cannot go on without a GUI.
    exit (1);
  }

  // Autoconnect signal handler using our own connect function that
  // will pass this as 'data' argument in callback handler. It will then
  // allow us to call object callback from C callback function.
  glade_xml_signal_autoconnect_full (_xml, GUI_connect, this);

  return true;
}

// ===================================================================
// = show ============================================================
/**
   Make a specific widget visible
   @param name the name of the widget to show
*/
// ===================================================================
void GUI::show (std::string name) {
  if (_xml)
    gtk_widget_show (get(name));
}


// ===================================================================
// = hide ============================================================
/**
   Make a specific widget invisible.
   @param name the name of the widget to hide
*/
// ===================================================================
void GUI::hide (std::string name) {
  if (_xml)
    gtk_widget_hide (get (name));
}

// ===================================================================
// = get =============================================================
/**
   Return the specified widget by looking into XML tree
   @param name is the name of the widget to get
   @return widget or 0 if not found
*/
// ===================================================================
GtkWidget *GUI::get (std::string name) {
  GtkWidget *widget = glade_xml_get_widget (_xml, (gchar *) name.c_str());
  if (!widget)
    g_warning (_("Cannot find the specified widget (\"%s\")"
               " within xml structure (\"%s\")"), name.c_str(), _xmlFilename.c_str());
  return widget;
}

// ===================================================================
// = on_delete =======================================================
/**
   Handle the delete event by hiding the widget
   @param widget is the widget where the delete event occured
   @param event is the delete event
   @return true indicating we've taking care of the event and don't
   want to be deleted
*/
// ===================================================================
gboolean GUI::on_delete (GtkWidget *widget, GdkEvent *event) {
  gtk_widget_hide (widget);
  return true;
}

// ===================================================================
// = on_destroy ======================================================
/**
   Handle the destroy event by quitting the application
   @param widget is the widget where the destroy event occured
   @param event is the destroy event
   @return true indicating we've taking care of the event
*/
// ===================================================================
gboolean GUI::on_destroy (GtkWidget *widget, GdkEvent *event) {
  gtk_main_quit();
  return true;
}

// ===================================================================
// = GUI_connect =====================================================
/**
   Use gmodule to connect signals automatically.
   Basically a symbol with the name of the signal handler is
   searched for, and that is connected to the associated symbols.
   So setting gtk_main_quit as a signal handler for the destroy
   signal of a window will do what you expect. This function is
   internally used and should not be used directly.
*/
// ===================================================================
void GUI_connect (const gchar *handler_name, GObject *object,
		  const gchar *signal_name, const gchar *signal_data,
		  GObject *connect_object, gboolean after,
		  gpointer user_data) {
  GModule *symbols = 0;
  GtkSignalFunc func;
 
  if (!symbols) {
    if (!g_module_supported()) {
      g_error (_("GUI_connect requires working gmodule"));
      exit (1);
    }
    symbols = g_module_open (0, G_MODULE_BIND_MASK);
  }
 
  if (!g_module_symbol(symbols, handler_name, (gpointer *) &func))
    g_warning(_("Could not find signal handler '%s'."), handler_name);
  else
    g_signal_connect (object, signal_name, GTK_SIGNAL_FUNC(func), user_data);
}

// ===================================================================
// = utf8_to_filename ================================================
/**
   Convert an utf8 filename string to a filename string
   @param text is the text to convert
   @return converted text
*/
// ===================================================================
std::string GUI::utf8_to_filename (std::string text) {
  gchar *buffer = g_filename_from_utf8 (text.c_str(),-1,0,0,0);
  std::string newtext (buffer);
  g_free (buffer);
  return newtext;
}

// ===================================================================
// = utf8_to_locale ==================================================
/**
   Convert an utf8 locale string to a locale string
   @param text is the text to convert
   @return converted text
*/
// ===================================================================
std::string GUI::utf8_to_locale (std::string text) {
  gchar *buffer = g_locale_from_utf8 (text.c_str(),-1,0,0,0);
  std::string newtext (buffer);
  g_free (buffer);
  return newtext;
}

// ===================================================================
// = filename_to_utf8 ================================================
/**
   Convert a filename string to an utf8  string
   @param text is the text to convert
   @return converted text
*/
// ===================================================================
std::string GUI::filename_to_utf8 (std::string text) {
  gchar *buffer = g_filename_to_utf8 (text.c_str(),-1,0,0,0);
  std::string newtext (buffer);
  g_free (buffer);
  return newtext;
}

// ===================================================================
// = locale_to_utf8 ==================================================
/**
   Convert a locale string to an utf8 string
   @param text is the text to convert
   @return converted text
*/
// ===================================================================
std::string GUI::locale_to_utf8 (std::string text) {
  gchar *buffer = g_locale_to_utf8 (text.c_str(),-1,0,0,0);
  std::string newtext (buffer);
  g_free (buffer);
  return newtext;
}
