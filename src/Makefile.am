## Process this file with automake to produce Makefile.in

gnubiffdir = $(datadir)/gnubiff
INCLUDES = \
	$(GNUBIFF_DEP_CFLAGS)				\
	$(GNOME_PANEL_DEP_CFLAGS)			\
	-DGNUBIFF_LOCALEDIR=\""$(datadir)/locale"\"	\
	-DGNUBIFF_DATADIR=\""$(gnubiffdir)"\"		\
	-DGNUBIFF_ICONDIR=\""$(datadir)/pixmaps"\"	\
	-DREAL_DATADIR=\""$(datadir)"\"			\
	-DG_DISABLE_DEPRECATED				\
	-DGDK_DISABLE_DEPRECATED			\
	-DGTK_DISABLE_DEPRECATED			\
	-DGDK_PIXBUF_DISABLE_DEPRECATED			\
	-DGNOME_DISABLE_DEPRECATED			\
	-DGNOME_VFS_DISABLE_DEPRECATED			\
	-DLIBGLADE_DISABLE_DEPRECATED			\
	-D_GNU_SOURCE					\
	-Wall						\
	-I/usr/include/kerberos/			\
	-I/usr/include/kerberosIV/

bin_PROGRAMS = gnubiff

gnome_sources = \
	AppletGNOME.cc AppletGNOME.h

common_sources = \
	gnubiff.cc					\
	AppletGTK.cc AppletGTK.h	\
	GUI.cc GUI.h				\
	gtk_image_animation.cc gtk_image_animation.h \
	Biff.cc Biff.h				\
	Setup.cc Setup.h			\
	Applet.cc Applet.h			\
	Popup.cc Popup.h			\
	Mailbox.cc Mailbox.h		\
	File.cc File.h				\
	Mh.cc Mh.h					\
	Maildir.cc Maildir.h		\
	Imap4.cc Imap4.h			\
	Pop.cc Pop.h				\
	Pop3.cc Pop3.h				\
	Apop.cc Apop.h


if USE_GNOME
gnubiff_SOURCES = \
	$(gnome_sources)		\
	$(common_sources)
else
gnubiff_SOURCES = \
	$(common_sources)
endif


gnubiff_LDADD = $(INTLLIBS) $(GNUBIFF_DEP_LIBS) $(GNOME_PANEL_DEP_LIBS)

gnubiff_DATA = \
	Setup.glade		\
	AppletGTK.glade		\
	AppletGNOME.glade	\
	Popup.glade


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
	AppletGTK.glade					\
	AppletGNOME.glade				\
	Setup.glade						\
	Popup.glade						\
	GNOME_gnubiffApplet.server.in	\
	GNOME_gnubiffApplet.xml			\
	gnubiff.schemas					\
	$(gnome_sources)

DISTCLEANFILES = \
        GNOME_gnubiffApplet.server
