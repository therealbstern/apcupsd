topdir:=../..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

all-install: install-mingw
all-uninstall: uninstall-mingw

install-mingw:
	$(call DISTINST,mingw)
	@echo "You have to manually install apcupsd boot script and"
	@echo "halt script for clean emergency shutdown."
	@echo "Please contribute your distribution install to apcupsd team."
	@echo "I'm sorry: you are on your own."

uninstall-mingw:
	$(call DISTUNINST,mingw)
	@echo "You have to manually uninstall apcupsd boot script and"
	@echo "halt script."
	@echo "Please contribute your distribution install to apcupsd team."
	@echo "I'm sorry: you are on your own."

TEXTFILES := $(topdir)/COPYING \
             $(topdir)/ChangeLog \
             $(topdir)/ReleaseNotes \
             $(wildcard $(topdir)/doc/*.txt)

MAKENSIS = makensis
UNIX2DOS = unix2dos

winapcupsd.exe:
	$(STRP) $(wildcard $(topdir)/src/*.exe) $(wildcard $(topdir)/src/win32/*.exe) $(wildcard $(topdir)/src/cgi/*.cgi)
	$(UNIX2DOS) -k -n $(foreach f,$(TEXTFILES),$(f) installer/$(subst .txt,,$(notdir $(f))).txt)
	$(MAKENSIS) -V2 -DDEPKGS="$(DEPKGS)" -DCROSSTOOLS="$(CROSSTOOLS)" -DVERSION="$(VERSION)" -DTOPDIR="${topdir}" installer/winapcupsd.nsi