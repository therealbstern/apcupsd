topdir:=../..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

# PackageMaker variables
PACKAGEMAKER=/Developer/Tools/packagemaker
PKGROOT=/tmp/apcupsd-pkg

all-install: install-darwin install-$(USBDRV)
all-uninstall: uninstall-darwin

install-darwin:
	$(call DISTINST,Darwin Mac OS X)
	$(call MKDIR,/Library/StartupItems/apcupsd)
	$(call INSTDATA,744,apcupsd,/Library/StartupItems/apcupsd)
	$(call INSTDATA,444,StartupParameters.plist,/Library/StartupItems/apcupsd)
	-$(V)chown root:wheel $(DESTDIR)/Library/StartupItems/apcupsd
	-$(V)chown root:wheel $(DESTDIR)/Library/StartupItems/apcupsd/apcupsd
	-$(V)chown root:wheel $(DESTDIR)/Library/StartupItems/apcupsd/StartupParameters.plist
	@echo "================================================="
	@echo " "
	@echo "apcupsd script installation for MacOS X (Darwin) complete." 
	@echo " "
	@echo "You should now edit $(DESTDIR)/$(sysconfdir)/apcupsd.conf to correspond"
	@echo "to your setup then start the apcupsd daemon with:"
	@echo " "
	@echo "    SystemStarter start \"APC UPS monitor\""
	@echo " "
	@echo "Thereafter when you reboot, it will be stopped and started"
	@echo "automatically."
	@echo " "
	@if [ -f $(DESTDIR)/$(sysconfdir)/apcupsd ] ; then \
		echo "WARNING: Old startup script $(DESTDIR)/$(sysconfdir)/apcupsd was renamed to"; \
		echo "         $(DESTDIR)/$(sysconfdir)/apcupsd.obsolete. Be sure to remove any";   \
		echo "         references to that script that you may have manually";    \
		echo "         added to the system init scripts. Apcupsd startup is";    \
		echo "         now managed via SystemStarter, making the old script";    \
		echo "         obsolete.";                                               \
		echo " ";                                                                \
		mv $(DESTDIR)/$(sysconfdir)/apcupsd $(DESTDIR)/$(sysconfdir)/apcupsd.obsolete;                 \
	fi
	@echo "================================================="

install-usb:
	$(call MKDIR,/System/Library/Extensions/ApcupsdDummy.kext/Contents)
	-$(V)chown root:wheel $(DESTDIR)/System/Library/Extensions/ApcupsdDummy.kext
	-$(V)chown root:wheel $(DESTDIR)/System/Library/Extensions/ApcupsdDummy.kext/Contents
	$(V)chmod 755 $(DESTDIR)/System/Library/Extensions/ApcupsdDummy.kext
	$(V)chmod 755 $(DESTDIR)/System/Library/Extensions/ApcupsdDummy.kext/Contents
	$(call INSTDATA,644,Info.plist,/System/Library/Extensions/ApcupsdDummy.kext/Contents)
	-$(V)chown root:wheel $(DESTDIR)/System/Library/Extensions/ApcupsdDummy.kext/Contents/Info.plist
	$(call UNINST,/System/Library/Extensions.mkext)
	$(call UNINST,/System/Library/Extensions.kextcache)
	@echo "================================================="
	@echo " "
	@echo "Driver installation complete." 
	@echo "You must REBOOT before running apcupsd."
	@echo " "
	@echo "================================================="

uninstall-darwin:
	$(call DISTUNINST,Darwin Mac OS X)
	-$(call UNINST,/Library/StartupItems/apcupsd)
	-$(call UNINST,/System/Library/Extensions/ApcupsdDummy.kext)
	-$(call UNINST,/System/Library/Extensions.mkext)
	-$(call UNINST,/System/Library/Extensions.kextcache)
	@echo "================================================="
	@echo " "
	@echo "Please REBOOT to complete uninstall." 
	@echo " "
	@echo "================================================="

# NOTE: The seeminly-pointless "PWD=`pwd`" stuff below is to work around
# a packagemaker bug in XCode-2.5.
# <http://lists.apple.com/archives/Installer-dev/2007/Jun/msg00061.html>
apcupsd.pkg:
	rm -rf $(PKGROOT)
	mkdir $(PKGROOT)
	mkdir $(PKGROOT)/ApcupsdDaemon.Root
	mkdir $(PKGROOT)/ApcupsdUsbShim.Root
	mkdir $(PKGROOT)/ApcupsdDaemon.Resources
	mkdir $(PKGROOT)/ApcupsdUsbShim.Resources
	mkdir $(PKGROOT)/ApcupsdPkg.Resources
	mkdir $(PKGROOT)/Packages
	mkdir $(PKGROOT)/Apcupsd
	( cd $(topdir) && DESTDIR=$(PKGROOT)/ApcupsdDaemon.Root make install )
	mv $(PKGROOT)/ApcupsdDaemon.Root/System $(PKGROOT)/ApcupsdUsbShim.Root
	cp $(topdir)/ReleaseNotes $(PKGROOT)/ApcupsdPkg.Resources/ReadMe.txt
	cp $(topdir)/COPYING $(PKGROOT)/ApcupsdPkg.Resources/License.txt
	cp Welcome.txt $(PKGROOT)/ApcupsdPkg.Resources
	cp ApcupsdDaemon.preflight $(PKGROOT)/ApcupsdDaemon.Resources/preflight
	cp ApcupsdDaemon.postflight $(PKGROOT)/ApcupsdDaemon.Resources/postflight
	cp ApcupsdUsbShim.postflight $(PKGROOT)/ApcupsdUsbShim.Resources/postflight
	PWD=`pwd` $(PACKAGEMAKER) -build -v -p $(PKGROOT)/Packages/ApcupsdDaemon.pkg -f $(PKGROOT)/ApcupsdDaemon.Root -r $(PKGROOT)/ApcupsdDaemon.Resources -i ApcupsdDaemon.Info.plist -d ApcupsdDaemon.Description.plist
	PWD=`pwd` $(PACKAGEMAKER) -build -v -p $(PKGROOT)/Packages/ApcupsdUsbShim.pkg -f $(PKGROOT)/ApcupsdUsbShim.Root -r $(PKGROOT)/ApcupsdUsbShim.Resources -i ApcupsdUsbShim.Info.plist -d ApcupsdUsbShim.Description.plist
	PWD=`pwd` $(PACKAGEMAKER) -build -v -p $(PKGROOT)/Apcupsd/Apcupsd-$(VERSION).pkg -mi $(PKGROOT)/Packages -r $(PKGROOT)/ApcupsdPkg.Resources -i ApcupsdPkg.Info.plist -d ApcupsdPkg.Description.plist
	hdiutil create -ov -srcfolder $(PKGROOT)/Apcupsd -volname Apcupsd-$(VERSION) Apcupsd-$(VERSION).dmg

