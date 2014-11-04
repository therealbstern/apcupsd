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
