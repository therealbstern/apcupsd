Name: gapcmon
Version: 0.7.0
Release: 0
License: GPL
Group: Applications/System
Source: gapcmon-%{version}.tar.bz2
URL: http://gapcmon.sourceforge.net/
Provides: gapcmon
AutoReqProv: no
Prefix:	/usr
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Summary: A gtk gui program, with support for the FreeDesktop.org notification area icons, to monitor the operation of UPS's being controlled by the APCUPSD package

%description
gapcmon monitors and displays the status 
of UPSs under the management of the APCUPSD package. 
APCUPSD.sourceforge.net is required, as is the NIS api 
of apcupsd, which may need top be compiled in. 

%prep
%setup -q

%build
./configure --prefix="%{prefix}" --disable-maintainer-mode
make "%{?_smp_mflags}" RPM_OPT_FLAGS="%{optflags}" all

%install
rm -rf %{buildroot}
make "%{?_smp_mflags}" prefix="%{prefix}" DESTDIR="%{buildroot}" install

%clean
rm -rf "%{buildroot}"
make "%{?_smp_mflags}" clean

%files
%defattr(-,root,root,-)
%doc %{_docdir}/gapcmon/README
%doc %{_docdir}/gapcmon/AUTHORS
%doc %{_docdir}/gapcmon/COPYING
%doc %{_docdir}/gapcmon/NEWS
%doc %{_docdir}/gapcmon/INSTALL
%doc %{_docdir}/gapcmon/ChangeLog
%doc %{_docdir}/gapcmon/gapcmon.desktop
%doc %{_datadir}/pixmaps/apcupsd.png
%doc %{_datadir}/pixmaps/online.png
%doc %{_datadir}/pixmaps/onbatt.png
%doc %{_datadir}/pixmaps/charging.png
%doc %{_datadir}/pixmaps/unplugged.png
%doc %{_datadir}/pixmaps/gapc_prefs.png
%doc %{_datadir}/applications/gapcmon.desktop
%{_bindir}/gapcmon


%changelog
* Tue Apr 18 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- gapc_devel (0.6.7-0) stable; urgency=normal
- Rewrite to include desktop independant features completed.
- This module replace all other interations of gapcmon: including
  gapc, gpanel, gpanel_apcmon, etc.

* Thu Apr 6 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- Implemented the FreeDesktop.org spec for notification_area icons
  using modified source from gnome.cvs:libegg.
- Added a paned window to help user manage size of icons and notebooks
- Attempted to implement true alphabetic sort of icons labels, by adding
  label text to main.window icons ( not working the why I would like )
- Reviewed gdk_threads_enter() useage and removed duplicates from
  gtkglgraph.c in response to a user bug ( no x|y-labels on chart-N+1)  
- Several routine consolidated or parameter-ized.

* Sat Apr 1 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- Implemented the tooltip in line graph history page.  Now displays
  the colored point value under the mouse.
- Updated the gapc_monitor_update() in gapcmon_core.c to handle
  the 0-100 percent scale for charted data points.  LINEV uses
  HITRANS as scale and defaults to 120:230, BATTV uses 12:24 unless 
  NOMBATTV is present, and LOADPCT, TIMELEFT, are inversly related to
  each other.  BCHARGE values are taken as is.
- Updated the gapc_util_point_filter_set() in gapcmon_core.c to handle
  out of range graphing data points by defaulting them to 108 percent
  or zero percent.

* Fri Mar 31 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- Implemented the sort_icons function to reorder dynamically
  created monitor icons.
- Added gtk_glgraph_unrealize() and gtk_glgraph_destroy() to
  the gapcmon_gtkglgraph subpackage.  To fix multi-threaded
  operations.
- Integrated the source files into a shared arrangement.
  gapcmon_core.[c|h] gapcmon_gtkglgraph.[c|h] and two
  anchor files gapcmon.h and gpanel_apcmon.c
- Added gtk_glgraph feature to gapcmon replacing the progress
  bars on the overview page.
- Added support for up to eight monitors to gapcmon with the 
  full functionality of gpanel.  commandline take one value
  the instance number, which defaults to zero.
- Reordered preferences columns for better presentation.

* Fri Mar 17 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- Implemented the none-enabled and COMMLOST program states

* Thu Mar 16 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- gapc_devel (0.5.7-2) stable; urgency=normal
- Modified gpanel_apcmon.c to handle an unexpected intial state issue
  and some stalled Icon updates issue.
- none-enabled relates to the very first execution without
  gconf2 schemas installed to supply defaults.  This resulted in
  no icons or monitor being enabled, and thus no access to the 
  user interface to request one; or a condition where one icon
  is present but dis-functional.  Fixed
- stalled icons relate to a state where the user has selected
  and repeated deseleted a particular monitor.  The icon for 
  that monitor will default to OFFLINE and not change until
  the UPS actually changes state like to charging, or on battery.
  Fixed.    

* Mon Mar 14 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- gapc_devel (0.5.7-0) stable; urgency=normal
- Modified gp_mon.c to use the GtkGLGraph package for line graphs.
  Caused two files containing GtkGLGraph to be added;
  gapcmon_gtkglgraph.c and .h The X11 libs are also needed
  by the openGL graphics package.
- gpanel_apcmon is now the only applet version available

* Fri Mar 10 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- Modified gp_mon to be compiled with and without Gtk+Extra-2.1.1
  installed.  gpanel_apcmon.c collection has been removed from
  distribution as gp_mon is a more stable codeset.
- GCONF is newly required for gp_mon to compile period.

* Mon Mar 06 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- Adding gp_mon an advanced panel applet with a histogram
  chart displaying a view of the last 400 collections.

* Wed Feb 15 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- gapc_devel (0.5.4-8) stable; urgency=normal
- Cleaned up desktop entry and rpm spec file
- Added --enable-gpanel and --enable-gapcmon feature selection to 
  the configure and Makefile scripts.

* Tue Feb 14 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- gapc_devel (0.5.4-7) stable; urgency=normal
- RPM Release to gapcmon-0.5.4-7.i686.rpm to sourceforge. The
  last one for a while thru t    Adding gp_mon an advanced panel applet with a histogram
    chart displaying a view of the last 400 collections.
  he gapcmon.sourceforge.net.
  Switching to a feature under apcupsd.sourceforge.net
- Created a GNOME panel Applet version named gpanel_apcmom
- Merged the codesets of gapcmon and gpanel_apcmon
  to produce one distribution: This one.

* Tue Jan 31 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- Added refresh interval selection to config page to allow user
  to select the number of seconds between data refreshes.
- Added configure script to support this and future distributions.

* Sun Jan 29 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- reworked configuration file routines to remove GKeyFile depends
  which was a glib 2.6 feature.  now using glib 2.4 g_io_channel...

* Sat Jan 28 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- reworked the chart calc for remaining time to better represent
- tweaked alignment of labels on information page.

* Fri Jan 27 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- fixed a bug in parse_args that would cause a hard loop if cmdline
  parms were supplied on program start.

* Thu Jan 26 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- reworked configuration file routine to use GKeyFile...

* Wed Jan 25 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- changed makefile to add gnome-vfs-module-2.0 to gtk compile flags
- reworked network socket routines to use GnomeVFS for socket programming
- general cleanup to enforce glib/gtk standards and remove most libc routines
- goal is to enhance design for portability

* Fri Jan 20 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- Cleanup network error messages
- Add pango markup support to the custom barchart on info page.

* Thu Jan 19 2006 James Scott, Jr. <skoona@users.sourceforge.net>
- Cleanup thread support in timer routiners
- Initial Public Release
