Summary: Mail notification program for GNOME 2
Name: gnubiff
Version: 1.4.0
Release: 1
Copyright: GPL
Group: X11/Utilities
Source: %{name}-%{version}.tar.bz2
URL: http://gnubiff.sourceforge.net
Prefix: /usr
Buildroot: %{_tmppath}/%{name}-root
Packager: Nicolas Rougier <rougier@loria.fr>
BuildRequires: libglade2-devel, libglade2 >= 2.3, gtk2-devel >= 2.4


# description take from the website (see URL)
%description
gnubiff is a mail notification program that checks for mail, displays
headers when new mail has arrived and allow to read first lines of
new mails. It relies on the GNOME and GTK libraries but can be
compiled and used with or without GNOME support. Supported protocols
are pop3, apop, imap4, mh, qmail and mailfile. Furthermore, gnubiff
is fully configurable with a lot of options like polltime, poptime,
sounds, mail reader, etc.

%clean
case "$RPM_BUILD_ROOT" in *-root) rm -rf $RPM_BUILD_ROOT ;; esac

%prep
%setup
CPPFLAGS="-I/usr/kerberos/include" \
	./configure --prefix=%{prefix} --with-password

%build
make

%install
case "$RPM_BUILD_ROOT" in *-root) rm -rf $RPM_BUILD_ROOT ;; esac
make install prefix=$RPM_BUILD_ROOT%{prefix}

%files 
%defattr(-,root,root)
%doc ABOUT-NLS AUTHORS COPYING ChangeLog INSTALL NEWS README THANKS
%{prefix}/share/gnubiff
%{prefix}/share/locale/*/LC_MESSAGES/gnubiff.mo
%{prefix}/share/sounds/gnubiff/
%{prefix}/share/pixmaps/gnubiff_icon.png
%{prefix}/bin/gnubiff
%doc %{prefix}/info/gnubiff*
%doc %{prefix}/man/man?/gnubiff*

%changelog
* Sat June 18 2004      rougier <rougier@loria.fr>
- RPM specfile for version 1.4.0
* Sun May 2 2004        rougier <rougier@loria.fr>
- RPM specfile for version 1.2.0
* Wed March 30 2004     rougier <rougier@loria.fr>
- RPM specfile for version 1.0.10
* Wed Jan 07 2004       rougier <rougier@loria.fr>
- RPM specfile for version 1.0.9
* Wed Nov 20 2003       rougier <rougier@loria.fr>
- RPM specfile for version 1.0.6
* Wed Sep 03 2003       Settel <rpm-gnubiff@m1.sirlab.de>
- Initial RPM specfile for version 1.0.1
