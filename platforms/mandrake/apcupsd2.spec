# Spec-File for apcupsd under Mandrake 9.0
#
%define name apcupsd
%define ver 3.10.4
%define rel 1
%define _confdir /etc/apcupsd
%define _sbindir /sbin
%define _cgidir /var/www/cgi-bin/apcupsd
%define _mandir /usr/share/man

Summary : Steuerungssoftware für APC USVs mit seriellem und USB-Port
Name: %{name}
Version: %{ver}pre1
Release: %{rel}
Copyright: GPL
Vendor: Linux New Media AG
Packager: Mirko Dölle <mdoelle@linux-user.de>
Source: apcupsd-3.10.4pre1.tar.gz
URL: http://www.apcupsd.com
Group: Applications/System
Provides: apcupsd
BuildRequires: libgd1-devel, libncurses5-devel, autoconf, automake
Requires: bash, freetype, freetype2, glibc, libgd1, libgpm1, libintl2, libjpeg62, libncurses5, libpng3, libxpm4, XFree86-libs, zlib1

%description
Der APCUPS-Daemon erlaubt die Überwachung der USV über ein serielles Kabel
oder USB-Anschluss. Zudem wird der Rechner bei Stromausfall automatisch
herunter gefahren, wenn die USV fast erschöpft ist. Dieses Paket ist für
USB-USVs von APC vorgesehen (Back-UPS CS und ES), kann aber auch für seriell
angesteuerte Modelle benutzt werden.

%prep
%setup -q

%build
 
%configure \
  --sysconfdir=%{_confdir} \
  --mandir=%{_mandir} \
  --sbindir=%{_sbindir} \
  --with-serial-dev=/dev/usb/hid/hiddev[0-9] \
  --with-upstype=usb \
  --with-upscable=usb \
  --enable-pthreads \
  --enable-usb \
  --enable-powerflute \
  --enable-cgi \
  --with-cgi-bin=%{_cgidir}

%configure \
  --sysconfdir=%{_confdir} \
  --mandir=%{_mandir} \
  --sbindir=%{_sbindir} \
  --with-serial-dev=/dev/usb/hid/hiddev[0-9] \
  --with-upstype=usb \
  --with-upscable=usb \
  --enable-pthreads \
  --enable-usb \
  --enable-powerflute \
  --enable-cgi \
  --with-cgi-bin=%{_cgidir}

make

%install
make install

%post
mkdir -p /dev/usb/hid
for ((i=0; i<16; i=$[$i+1])); do
  if [ ! -e /dev/usb/hid/hiddev${i} -a ! -c /dev/usb/hid/hiddev${i} ]; then
    mknod /dev/usb/hid/hiddev${i} c 180 $[96+$i]
  fi
done

%files
%{_sbindir}/apcaccess
%{_sbindir}/apcnisd
%{_sbindir}/apcupsd
%{_sbindir}/powerflute
%{_cgidir}
%{_confdir}
%{_mandir}/man8/apcupsd.8
/etc/init.d/apcupsd
/etc/rc.d/rc0.d/K99apcupsd
/etc/rc.d/rc1.d/K99apcupsd
/etc/rc.d/rc2.d/S20apcupsd
/etc/rc.d/rc3.d/S20apcupsd
/etc/rc.d/rc4.d/S20apcupsd
/etc/rc.d/rc5.d/S20apcupsd
/etc/rc.d/rc6.d/K99apcupsd
