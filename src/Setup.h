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
#ifndef _SETUP_H
#define _SETUP_H

#ifdef HAVE_CONFIG_H
#   include "../config.h"
#endif
#include "GUI.h"


class Setup : public GUI {

  // ===================================================================
  // - Public methods --------------------------------------------------
  // ===================================================================
public:
	Setup (class Biff *owner, std::string xmlFilename);
  ~Setup (void);

  gint create (void);
  void update (void);
  void updateOwner (void);

  void show (std::string name = "dialog");
  void hide (std::string name = "dialog");
  gboolean on_delete (GtkWidget *widget,  GdkEvent *event);
  void on_cancel (void);
  void on_save (void);
  void on_quit (void);
  void on_suspend (void);
  void on_tree_select (GtkTreeSelection *selection);
  void on_protocol (void);
  void on_sound (void);
  void on_misc_change (void);
  void on_browse_address (void);
  void on_browse_certificate (void);
  void on_browse_sound (void);
  void on_play_sound (void);
  void on_browse_mail_app (void);
  void on_browse_mail_image (void);
  void on_browse_nomail_image (void);
  void on_browse_font (std::string name);
  void on_browse_color (std::string name);
  void on_new_mailbox (void);
  void on_delete_mailbox (void);
  void on_prev_mailbox (void);
  void on_next_mailbox (void);


  // ===================================================================
  // - Private methods -------------------------------------------------
  // ===================================================================
private:
  void addTreeBranch (GtkTreeStore *tree,
					  gchar *notebook_label,
					  const gchar *icon,
					  gchar *tree_label,
					  GtkTreeIter *parent,
					  GtkTreeIter *iter,
					  gint page_index);


  // ===================================================================
  // - Private attributes ----------------------------------------------
  // ===================================================================
private:
  class Biff *		_owner;		// Owner of this interface
  int				_current;	// Current displayed mailbox
};

#endif
