topdir:=..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

all-install: install-man
all-uninstall: uninstall-man

install-man:
	$(call MKDIR,$(mandir)/man8)
	$(call INSTDATA,644,apcupsd.8,$(mandir)/man8/apcupsd.8)
	$(call INSTDATA,644,apcaccess.8,$(mandir)/man8/apcaccess.8)
	$(call INSTDATA,644,apctest.8,$(mandir)/man8/apctest.8)
	$(call INSTDATA,644,apccontrol.8,$(mandir)/man8/apccontrol.8)
	$(call MKDIR,$(mandir)/man5)
	$(call INSTDATA,644,apcupsd.conf.5,$(mandir)/man5/apcupsd.conf.5)

uninstall-man:
	$(call UNINST,$(mandir)/man8/apcupsd.8)
	$(call UNINST,$(mandir)/man8/apcaccess.8)
	$(call UNINST,$(mandir)/man8/apctest.8)
	$(call UNINST,$(mandir)/man8/apccontrol.8)
	$(call UNINST,$(mandir)/man5/apcupsd.conf.5)
