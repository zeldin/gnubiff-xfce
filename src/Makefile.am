INCLUDES = \
	$(GNUBIFF_DEP_CFLAGS)						\
	$(GNOME_PANEL_DEP_CFLAGS)					\
	-DGNUBIFF_LOCALEDIR=\""$(datadir)/locale"\"	\
	-DGNUBIFF_DATADIR=\""$(pkgdatadir)"\"		\
	-DGNUBIFF_ICONDIR=\""$(datadir)/pixmaps"\"	\
	-DREAL_DATADIR=\""$(datadir)"\"				\
	-D_GNU_SOURCE								\
	-Wall -ansi -pedantic

bin_PROGRAMS = gnubiff

gnome_sources =  \
	ui-applet-gnome.cc 		ui-applet-gnome.h

common_sources = \
	gnubiff.cc										\
	biff.cc					biff.h					\
							nls.h					\
	\
	mailbox.cc				mailbox.h				\
	socket.cc				socket.h				\
	file.cc					file.h					\
	imap4.cc				imap4.h					\
	maildir.cc				maildir.h				\
	mh.cc					mh.h					\
	pop.cc					pop.h					\
	apop.cc					apop.h					\
	pop3.cc					pop3.h					\
	\
	gui.cc					gui.h					\
	ui-applet.cc			ui-applet.h				\
	ui-applet-gtk.cc 		ui-applet-gtk.h			\
	ui-preferences.cc 		ui-preferences.h		\
	ui-authentication.cc	ui-authentication.h		\
	ui-certificate.cc		ui-certificate.h		\
	ui-properties.cc		ui-properties.h			\
	gtk_image_animation.cc	gtk_image_animation.h 	\
	ui-popup.cc				ui-popup.h				\
	support.cc				support.h

if USE_GNOME
gnubiff_SOURCES = \
	$(gnome_sources)		\
	$(common_sources)
else
gnubiff_SOURCES = \
	$(common_sources)
endif


gnubiff_LDADD = $(INTLLIBS) $(GNUBIFF_DEP_LIBS) $(GNOME_PANEL_DEP_LIBS)


serverdir       = $(libdir)/bonobo/servers
server_in_files = 
server_DATA     = 
uidir			= $(datadir)/gnome-2.0/ui
ui_DATA			= 

if USE_GNOME
server_in_files += GNOME_gnubiffApplet.server.in
server_DATA     += $(server_in_files:.server.in=.server)
ui_DATA			+= GNOME_gnubiffApplet.xml
else
endif

@INTLTOOL_SERVER_RULE@

EXTRA_DIST = \
	GNOME_gnubiffApplet.server.in	\
	GNOME_gnubiffApplet.xml			\
	gnubiff.schemas					\
	$(gnome_sources)

DISTCLEANFILES = \
        GNOME_gnubiffApplet.server
