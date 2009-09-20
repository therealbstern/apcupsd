topdir:=../..
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.m)

LIBS := -Wl,-framework -Wl,Cocoa $(APCLIBS) $(LIBS)

all-targets: apcagent MainMenu.nib apcagent.app

apcagent: $(OBJS)
	$(LINK)

apcagent.app: apcagent MainMenu.nib Info.plist PkgInfo \
              $(topdir)/doc/images/commlost.png \
              $(topdir)/doc/images/charging.png \
              $(topdir)/doc/images/onbatt.png \
              $(topdir)/doc/images/online.png
	@$(ECHO) "  BUNDL" $@
	$(V)$(RMF) apcagent.app
	$(V)$(MKINSTALLDIRS) apcagent.app/Contents/MacOS
	$(V)$(MKINSTALLDIRS) apcagent.app/Contents/Resources
	$(V)$(MKINSTALLDIRS) apcagent.app/Contents/Resources/English.lproj
	$(V)$(INSTALL_PROGRAM) $(STRIP) -m 755 apcagent apcagent.app/Contents/MacOS
	$(V)$(SED) -e "s/__VERSION__/${VERSION}/" Info.plist > /tmp/Info.plist.out
	$(V)$(INSTALL_DATA) -m 644 /tmp/Info.plist.out apcagent.app/Contents/Info.plist
	$(V)$(INSTALL_DATA) -m 644 PkgInfo apcagent.app/Contents
	$(V)$(INSTALL_DATA) -m 644 MainMenu.nib apcagent.app/Contents/Resources/English.lproj
	$(V)$(INSTALL_DATA) -m 644 $(topdir)/doc/images/commlost.png apcagent.app/Contents/Resources
	$(V)$(INSTALL_DATA) -m 644 $(topdir)/doc/images/charging.png apcagent.app/Contents/Resources
	$(V)$(INSTALL_DATA) -m 644 $(topdir)/doc/images/onbatt.png apcagent.app/Contents/Resources
	$(V)$(INSTALL_DATA) -m 644 $(topdir)/doc/images/online.png apcagent.app/Contents/Resources

apcagent.dmg: apcagent.app
	hdiutil create -srcfolder apcagent.app apcagent.dmg

# Include dependencies
-include $(DEPS)
