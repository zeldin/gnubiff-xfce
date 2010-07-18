desktopdir       = $(datadir)/applications
desktop_in_files = gnubiff.desktop.in
desktop_DATA     = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

uidir            = $(pkgdatadir)
dist_ui_DATA     = $(wildcard *.glade)

EXTRA_DIST       = $(wildcard *.gladep) gnubiff.desktop.in

CLEANFILES	= $(desktop_DATA)
