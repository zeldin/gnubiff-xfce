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

#include "support.h"

#include <errno.h>
#include <signal.h>

#include "local.h"
#include "ui-applet.h"
#include "biff.h"


// ========================================================================
//  base
// ========================================================================	
Local::Local (Biff *biff) : Mailbox (biff)
{
	fam_connection_.fd = 0;
}

Local::Local (const Mailbox &other) : Mailbox (other)
{
	fam_connection_.fd = 0;
}

Local::~Local (void)
{
}


// ========================================================================
//  main
// ========================================================================	
void
Local::start (void)
{
	// is there already someone watching this mailbox ?
	if (!g_mutex_trylock (monitor_mutex_))
		return;
	
	// connection request to FAM (File Alteration Monitor)
	if (FAMOpen (&fam_connection_) < 0) {
		g_mutex_unlock (monitor_mutex_);
		return ;
	}

	// start monitoring
	if( FAMMonitorFile (&fam_connection_, address_.c_str(), &fam_request_, NULL) < 0) {
		FAMClose (&fam_connection_);
		g_mutex_unlock (monitor_mutex_);
		return ;
	}

	// at this point we need to explicitely call the get function since
	// monitoring will start from now on. Even if the mailbox was full,
	// no change appears ye, so we force it.
	fetch ();
	gdk_threads_enter();
	biff_->applet()->update();
	gdk_threads_leave();

	int status = 1;
	while (status == 1) {
		status = FAMNextEvent (&fam_connection_, &fam_event_);
		if( status < 0 ) {
			if( errno == EINTR )
				break;
			FAMClose (&fam_connection_);
			g_mutex_unlock (monitor_mutex_);
			return ;
		}
		
		if (fam_event_.code == FAMChanged) {
			fetch ();
			gdk_threads_enter();
			biff_->applet()->update();
			gdk_threads_leave();
		}

		else if (fam_event_.code == FAMAcknowledge)
			break;
	}

	FAMClose (&fam_connection_);
	g_mutex_unlock (monitor_mutex_);

	// Ok, we got an error, just retry montoring
	if (status != 1) {
		sleep (1);
		start ();
	}
}

void Local::stop (void)
{
	Mailbox::stop ();
	g_mutex_lock (monitor_mutex_);
	if (FAMCONNECTION_GETFD (&fam_connection_)) 
		FAMCancelMonitor (&fam_connection_, &fam_request_);
	g_mutex_unlock (monitor_mutex_);
}
