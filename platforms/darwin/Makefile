topdir:=../..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

all-install: install-darwin install-$(USBDRV)
all-uninstall: uninstall-darwin

install-darwin:
	$(call DISTINST,Darwin Mac OS X)
	$(call MKDIR,/Library/LaunchDaemons)
	$(call INSTDATA,644,org.apcupsd.apcupsd.plist,/Library/LaunchDaemons)
	-$(V)chown root:wheel $(DESTDIR)/Library/LaunchDaemons/org.apcupsd.apcupsd.plist
	$(call INSTDATA,755,apcupsd-uninstall,$(sbindir))
	$(call INSTDATA,755,apcupsd-start,$(sbindir))
	@echo "================================================="
	@echo " "
	@echo "apcupsd script installation for MacOS X (Darwin) complete." 
	@echo " "
	@echo "You should now edit $(DESTDIR)/$(sysconfdir)/apcupsd.conf to correspond"
	@echo "to your setup then start the apcupsd daemon with:"
	@echo " "
	@echo "   sudo launchctl load /Library/LaunchDaemons/org.apcupsd.apcupsd.plist"
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
	$(call MKDIR,/System/Library/Extensions/ApcupsdDummy.kext/Contents/_CodeSignature)
	-$(V)$(call INSTDATA,644,ApcupsdDummy.kext/Contents/Info.plist,/System/Library/Extensions/ApcupsdDummy.kext/Contents/Info.plist)
	-$(V)$(call INSTDATA,644,ApcupsdDummy.kext/Contents/_CodeSignature/CodeDirectory,/System/Library/Extensions/ApcupsdDummy.kext/Contents/_CodeSignature/CodeDirectory)
	-$(V)$(call INSTDATA,644,ApcupsdDummy.kext/Contents/_CodeSignature/CodeRequirements,/System/Library/Extensions/ApcupsdDummy.kext/Contents/_CodeSignature/CodeRequirements)
	-$(V)$(call INSTDATA,644,ApcupsdDummy.kext/Contents/_CodeSignature/CodeResources,/System/Library/Extensions/ApcupsdDummy.kext/Contents/_CodeSignature/CodeResources)
	-$(V)$(call INSTDATA,644,ApcupsdDummy.kext/Contents/_CodeSignature/CodeSignature,/System/Library/Extensions/ApcupsdDummy.kext/Contents/_CodeSignature/CodeSignature)
	-$(V)chmod 755 $(DESTDIR)/System/Library/Extensions/ApcupsdDummy.kext
	-$(V)chmod 755 $(DESTDIR)/System/Library/Extensions/ApcupsdDummy.kext/Contents
	-$(V)chmod 755 $(DESTDIR)/System/Library/Extensions/ApcupsdDummy.kext/Contents/_CodeSignature
	-$(V)chown -R root:wheel $(DESTDIR)/System/Library/Extensions/ApcupsdDummy.kext
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
	-$(V)launchctl remove org.apcupsd.apcupsd
	-$(call UNINST,/System/Library/Extensions/ApcupsdDummy.kext)
	-$(call UNINST,/System/Library/Extensions.mkext)
	-$(call UNINST,/System/Library/Extensions.kextcache)
	-$(call UNINST,$(sbindir)/apcupsd-uninstall)
	-$(call UNINST,$(sbindir)/apcupsd-start)
	-$(call UNINST,/Library/LaunchDaemons/org.apcupsd.apcupsd.plist)
	@echo "================================================="
	@echo " "
	@echo "Please REBOOT to complete uninstall." 
	@echo " "
	@echo "================================================="

# Root of package build sandbox
PKGROOT=/tmp/apcupsd-pkg

#
# Identity preferences for signing keys, set via Keychain Access utility or on
# command line using 'security set-identity-preference'
#
# APCUPSD_CODE_SIGNING - Code signing key
# APCUPSD_PACKAGE_SIGNING - Package signing key
#
# codesign searches by identity preference automatically
CODEKEY := APCUPSD_CODE_SIGNING
# productsign needs common name string, so look it up
PKGKEY := $(shell \
   security get-identity-preference -s APCUPSD_PACKAGE_SIGNING -c \
      | grep "common name" | cut -d\" -f2)

# Loose executables to sign
EXES := $(sbindir)/apcupsd $(sbindir)/apcaccess $(sbindir)/apctest $(sbindir)/smtp

# Make apcupsd installation package
apcupsd.pkg:
	rm -rf $(PKGROOT)
	mkdir $(PKGROOT)
	mkdir $(PKGROOT)/Apcupsd
	mkdir $(PKGROOT)/Apcupsd.Resources
	mkdir $(PKGROOT)/Apcupsd.Scripts
	mkdir $(PKGROOT)/Apcupsd.Root
	mkdir $(PKGROOT)/Apcagent.Scripts
	( cd $(topdir) && DESTDIR=$(PKGROOT)/Apcupsd.Root make install )
	cp $(topdir)/ReleaseNotes $(PKGROOT)/Apcupsd.Resources
	cp $(topdir)/COPYING $(PKGROOT)/Apcupsd.Resources
	cp Welcome.txt $(PKGROOT)/Apcupsd.Resources
	cp Conclusion.txt $(PKGROOT)/Apcupsd.Resources
	cp ApcupsdDaemon.preflight $(PKGROOT)/Apcupsd.Scripts/preinstall
	cp ApcupsdDaemon.postflight $(PKGROOT)/Apcupsd.Scripts/postinstall
	cp apcagent.postinstall $(PKGROOT)/Apcagent.Scripts/postinstall
	-chown root:wheel $(PKGROOT)/Apcupsd.Root/Library
	-chown root:wheel $(PKGROOT)/Apcupsd.Root/Library/LaunchDaemons
	-chmod 1775 $(PKGROOT)/Apcupsd.Root/Library
	-chmod 0775 $(PKGROOT)/Apcupsd.Root/Library/LaunchDaemons
	-codesign --sign $(CODEKEY) --timestamp --verbose --force --prefix com.apcupsd. $(addprefix $(PKGROOT)/Apcupsd.Root/,$(EXES)) && \
      codesign --verify --verbose $(addprefix $(PKGROOT)/Apcupsd.Root/,$(EXES))
	-codesign --sign $(CODEKEY) --timestamp --verbose --force $(PKGROOT)/Apcupsd.Root/Applications/apcagent.app && \
      codesign --verify --verbose $(PKGROOT)/Apcupsd.Root/Applications/apcagent.app
	pkgbuild --root $(PKGROOT)/Apcupsd.Root/Applications \
      --identifier com.apcupsd.apcagent --version $(VERSION) \
      --install-location /Applications --scripts $(PKGROOT)/Apcagent.Scripts \
      --component-plist component.plist $(PKGROOT)/apcagent.pkg
	rm -rf $(PKGROOT)/Apcupsd.Root/Applications
	pkgbuild --root $(PKGROOT)/Apcupsd.Root \
      --identifier com.apcupsd.apcupsd --version $(VERSION) \
      --install-location / --scripts $(PKGROOT)/Apcupsd.Scripts \
      $(PKGROOT)/apcupsd.pkg
	productbuild --distribution apcupsd.dist --package-path $(PKGROOT) \
      --resources $(PKGROOT)/Apcupsd.Resources --identifier org.apcupsd.apcupsd \
      --version $(VERSION) $(PKGROOT)/Apcupsd-$(VERSION).pkg
	-productsign --timestamp --sign "$(PKGKEY)" $(PKGROOT)/Apcupsd-$(VERSION).pkg $(PKGROOT)/Apcupsd/Apcupsd-$(VERSION).pkg
	hdiutil create -ov -fs HFS+ -srcfolder $(PKGROOT)/Apcupsd -volname Apcupsd-$(VERSION) Apcupsd-$(VERSION).dmg
