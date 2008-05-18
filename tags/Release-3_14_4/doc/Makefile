topdir:=..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

all-install: install-man
all-uninstall: uninstall-man

install-man:
	$(call MKDIR,$(mandir)/man$(manext))
	$(call INSTDATA,644,apcupsd.man,$(mandir)/man$(manext)/apcupsd.$(manext))

uninstall-man:
	$(call UNINST,$(mandir)/man$(manext)/apcupsd.$(manext))
