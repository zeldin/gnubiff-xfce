# gnubiff -- a mail notification program
# Copyright (C) 2000-2004 Nicolas Rougier
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this program; if not, write to the Free
# Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# Process this file with autoconf to produce a configure script.



######################################################################
#                           Initialization                           #
######################################################################

# gnubiff version numbering scheme:
#  gnubiff uses the  "major level", "minor level" & "patch level"
#  version numbering scheme
#
# - First number is the major number and it signifies major changes or
#   rewrites. 
# - The second number is the minor number and it represents added or
#    tweaked functionality on top of a largely coherent structure. 
# - The third number is the patch number and it usually will only
#    refer to releases fixing bugs
#
# Current version is 2.0.2

AC_PREREQ(2.53)
AC_INIT(gnubiff, 2.0.2, rougier@loria.fr)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_HEADERS([config/config.h])
AC_CONFIG_SRCDIR([src/gnubiff.cc])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

######################################################################
#                         Checks for programs                        #
######################################################################
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

######################################################################
#                         Checks for libraries                       #
######################################################################
AC_HAVE_LIBRARY(popt)
AC_HAVE_LIBRARY(ssl)
AC_CHECK_LIB(crypto, MD5_Init, AC_DEFINE(HAVE_CRYPTO, 1, Define to 1 if you have crypto base functions),
                [AC_CHECK_LIB(ssl, MD5_init, AC_DEFINE(HAVE_CRYPTO, 1, Define to 1 if you have crypto base functions))])
GTK_REQUIRED="gtk+-2.0 >= 2.4"
GDK_PIXBUF_REQUIRED="gdk-pixbuf-2.0 >= 2.4"
GMODULE_REQUIRED="gmodule-2.0 >= 2.4"
LIBGLADE_REQUIRED="libglade-2.0 >= 2.3"
GTHREAD="gthread-2.0 >= 2.4"
PKG_CHECK_MODULES(GNUBIFF_DEP, 
[
	$GTK_REQUIRED
	$GDK_PIXBUF_REQUIRED
	$GMODULE_REQUIRED
	$LIBGLADE_REQUIRED
	$GTHREAD
])
AC_SUBST(GNUBIFF_DEP_CFLAGS)
AC_SUBST(GNUBIFF_DEP_LIBS)


######################################################################
#                     Checks for header files                        #
######################################################################
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h libintl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h utime.h])



######################################################################
#   Checks for typedefs, structures, and compiler characteristics    #
######################################################################
AC_C_CONST



######################################################################
#                  Checks for library functions                      #
######################################################################
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_MALLOC
AC_FUNC_STAT
AC_FUNC_UTIME_NULL
AC_CHECK_FUNCS([gethostbyname memset socket strchr strerror strrchr strstr utime])



######################################################################
#                       Internationalisation                         #
######################################################################
GETTEXT_PACKAGE=gnubiff
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
                   [The prefix for our gettext translation domains.])
ALL_LINGUAS="fr fr_FR fr_CA de nl ro es sv it"
AC_PROG_INTLTOOL
AM_GLIB_GNU_GETTEXT


######################################################################
#                           GNOME support                            #
######################################################################
dnl syntaxt: AC_ARG_WITH (package, help-string, [action-if-given], [action-if-not-given])
dnl If the user gave configure the option `--with-package' or
dnl `--without-package', run shell commands action-if-given. If neither
dnl option was given, run shell commands action-if-not-given. The name
dnl package indicates another software package that this program should
dnl work with. It should consist only of alphanumeric characters and
dnl dashes.
AC_ARG_ENABLE(gnome,
[  --disable-gnome         disable use of gnome],
	gnome=$enableval, gnome=yes)

if test "$gnome" = yes; then
  PKG_CHECK_MODULES(GNOME_PANEL_DEP,libpanelapplet-2.0, [
	AC_SUBST(GNOME_PANEL_DEP_CFLAGS)
	AC_SUBST(GNOME_PANEL_DEP_LIBS)
	AH_TEMPLATE([USE_GNOME])
	AC_DEFINE(USE_GNOME)
	AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
	if test x"$GCONFTOOL" = xno; then
	   AC_MSG_ERROR([gconftool-2 executable not found in your path - should be installed with GConf])
	fi
  ], gnome=no)
fi
AM_CONDITIONAL(USE_GNOME, test $gnome = yes, Define to 1 if you want to have GNOME support)

######################################################################
#                        Save password option                        #
######################################################################
AC_ARG_WITH(password,
[  --with-password         Save password within configuration file (UNSECURE)],
        password=$withval, password=no)
if test "$password" = yes; then
	AH_TEMPLATE([USE_PASSWORD])
	AC_DEFINE(USE_PASSWORD)

    AC_ARG_WITH(password-string,
    [  --with-password-string  String to use to encrypt password (HIGHLY RECOMMENDED)],
        pstring=$withval, pstring="")
	AH_TEMPLATE([PASSWORD_STRING])
    if eval "test x$pstring = x"; then
		AC_DEFINE_UNQUOTED(PASSWORD_STRING, "FEDCBA9876543210")
    else
		AC_DEFINE_UNQUOTED(PASSWORD_STRING, "$withval")
    fi

fi
AM_CONDITIONAL(USE_PASSWORD, test "$password" = yes)
AM_CONDITIONAL(PASSWORD_STRING, test "$password" = yes)



######################################################################
#                           debug support                            #
######################################################################
AC_ARG_ENABLE(debug,
[  --enable-debug          turn on debugging [default=no]],
	AC_DEFINE(DEBUG, 1, [Define to 1 to enable debugging code.]),)



##################
# configure output
##################
AC_CONFIG_FILES([
Makefile
src/Makefile
po/Makefile.in
ui/Makefile
art/Makefile
sound/Makefile
doc/Makefile
man1/Makefile
])
AC_OUTPUT


echo "--------------------------------------------------------------------------------"
if test "$password" = yes; then
	echo " Password saving: yes"
	echo " Password string: $pstring"
	if test "x$pstring" = "x"; then
		echo "  WARNING: You've not defined a string to encrypt password (--with-password-string)"
		echo "           Default string will be used but it is less secure than using your own."
	fi
fi

if test "$password" = no; then
echo " Password saving: no"
echo "  If you want gnubiff to save password within configuration"
echo "  file, use ./configure --with-password [--with-password-string=\"any string you like...]\""
echo "  But be careful, it is highly unsecure, you've been warned !"
fi

echo " Gnome support: $gnome"
if test "$gnome" = "yes"; then
  if test "$prefix" != "`pkg-config libpanelapplet-2.0 --variable=prefix`"; then
    echo "  WARNING: prefix should be "`pkg-config libpanelapplet-2.0 --variable=prefix`
    echo "           but it is currently set to $prefix"
  fi
fi

echo ""
echo " Just type 'make' and then 'make install' to install gnubiff"
echo "--------------------------------------------------------------------------------"
echo ""
