# General targets for Makefile(s) subsystem.
# In this file we will put everything that need to be
# shared betweek all the Makefile(s).
# This file must be included at the beginning of every Makefile
#
# Copyright (C) 1999-2002 Riccardo Facchetti <riccardo@master.oasi.gpa.it>

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:

.SUFFIXES:
.SUFFIXES: .o .lo .c .cpp .h .po .gmo .mo .cat .msg .pox

.PHONY: all install uninstall install- install-apcupsd install-powerflute \
		install-cgi clean realclean distclean mostlyclean clobber

all: all-subdirs all-targets

all-subdirs:
	@if test ! -z "$(subdirs)"; then \
		for file in . ${subdirs}; \
		do \
			(cd $$file && if test "$$file" != "."; then make all; fi); \
			if test "$$?" != "0"; then \
				break; \
			fi; \
		done; \
	fi

# Standard compilation targets
dummy:

.c.o:
	$(CC) -c $(CFLAGS) $(DEFS) $<

.c.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) $<

.cpp.o:
	$(CC) -c $(CFLAGS) $(DEFS) $<

.po.pox:
	$(MAKE) $(PACKAGE).pot
	$(MSGMERGE) $< $(srcdir)/$(PACKAGE).pot -o $*.pox
 
.po.mo:
	$(MSGFMT) -o $@ $<
 
.po.gmo:
	file=$(srcdir)/`echo $* | sed 's,.*/,,'`.gmo \
	  && rm -f $$file && $(GMSGFMT) -o $$file $<
 
.po.cat:
	sed -f $(topdir)/src/intl/po2msg.sed < $< > $*.msg \
	  && rm -f $@ && $(GENCAT) $@ $*.msg					    

# Library targets
$(topdir)/src/lib/libapc.a: $(topdir)/src/lib/*.[ch]
	@(cd $(topdir)/src/lib && make)

$(topdir)/src/drivers/libdrivers.a: $(topdir)/src/drivers/*.[ch]
	@(cd $(topdir)/src/drivers && make)

$(topdir)/src/drivers/apcsmart/libapcsmart.a: $(topdir)/src/drivers/apcsmart/*.[ch]
	@(cd $(topdir)/src/drivers/apcsmart && make)

$(topdir)/src/drivers/dumb/libdumb.a: $(topdir)/src/drivers/dumb/*.[ch]
	@(cd $(topdir)/src/drivers/dumb && make)

$(topdir)/src/drivers/net/libnet.a: $(topdir)/src/drivers/net/*.[ch]
	@(cd $(topdir)/src/drivers/net && make)

$(topdir)/src/drivers/usb/libusb.a: $(topdir)/src/drivers/usb/*.[ch]
	@(cd $(topdir)/src/drivers/usb && make)

$(topdir)/src/drivers/snmp/libdumb.a: $(topdir)/src/drivers/snmp/*.[ch]
	@(cd $(topdir)/src/drivers/snmp && make)

$(topdir)/src/drivers/dumb/libtest.a: $(topdir)/src/drivers/test/*.[ch]
	@(cd $(topdir)/src/drivers/test && make)

$(topdir)/src/intl/libintl.a: $(topdir)/src/intl/*.[ch]
	@(cd $(topdir)/src/intl && make)

$(topdir)/src/gd1.2/libgd.a: $(topdir)/src/gd1.2/*.[ch]
	@(cd $(topdir)/src/gd1.2 && make)

$(topdir)/src/win32/winmain.o: $(topdir)/src/win32/winmain.cpp \
								$(topdir)/src/win32/winups.h
	@(cd $(topdir)/src/win32 && make winmain.o)

$(topdir)/src/win32/winlib.a: $(topdir)/src/win32/*.[ch] \
								$(topdir)/src/win32/*.cpp
	@(cd $(topdir)/src/win32 && make winlib.a)

$(topdir)/src/win32/winres.res: $(topdir)/src/win32/winres.rc \
								$(topdir)/src/win32/apcupsd.ico \
								$(topdir)/src/win32/winres.h \
								$(topdir)/src/win32/online.ico \
								$(topdir)/src/win32/onbatt.ico
	@(cd $(topdir)/src/win32 && make winres.res)

# Makefile subsystem targets
$(topdir)/autoconf/variables.mak: $(topdir)/autoconf/variables.mak.in
	@(cd $(topdir) && \
	SINGLE_MAKEFILE=yes \
	CONFIG_FILES=./autoconf/variables.mak \
	CONFIG_HEADERS= $(SHELL) ./config.status)

Makefiles:
	@(cd $(topdir) && \
	$(SHELL) ./config.status)

Makefile: $(srcdir)/Makefile.in $(topdir)/config.status \
			$(topdir)/autoconf/variables.mak $(topdir)/autoconf/targets.mak
	@(cd $(topdir) && \
	SINGLE_MAKEFILE=yes \
	CONFIG_FILES=$(RELCURDIR)Makefile \
	CONFIG_HEADERS= $(SHELL) ./config.status)
	@echo "You can ignore any makedepend error messages"
	@make depend

# Configuration targets

configure:  $(topdir)/autoconf/configure.in $(topdir)/autoconf/aclocal.m4 \
			$(topdir)/autoconf/acconfig.h $(topdir)/autoconf/config.h.in
	cd $(topdir);
	$(RMF) config.cache config.log config.out config.status include/config.h
	autoconf --localdir=$(topdir)/autoconf \
	autoconf/configure.in > configure
	chmod 755 configure

$(topdir)/autoconf/config.h.in: $(topdir)/autoconf/configure.in \
								$(topdir)/autoconf/acconfig.h
	cd $(srcdir);
	autoheader --localdir=$(srcdir)/autoconf \
	autoconf/configure.in > autoconf/config.h.in
	chmod 644 autoconf/config.h.in

$(topdir)/config.status:
	@if test -x $(topdir)/config.status; then \
		(cd $(topdir) && \
		$(SHELL) ./config.status --recheck); \
	else \
		(cd $(topdir) && \
		$(SHELL) ./configure); \
	fi

clean-subdirs:
	@if test ! -z "$(subdirs)"; then \
		for file in . ${subdirs}; \
		do \
			(cd $$file && if test "$$file" != "."; then make clean; fi); \
		done; \
	fi

distclean-subdirs:
	@if test ! -z "$(subdirs)"; then \
		for file in . ${subdirs}; \
		do \
			(cd $$file && if test "$$file" != "."; then make distclean; fi); \
		done; \
	fi

targetclean: clean-subdirs
	$(RMF) *.o *.lo *.a core core.* .*~ *~ *.bak
	$(RMF) *.exe *.res *.cgi
	$(RMF) 1 2 3 4 ID TAGS *.orig $(allexe)

targetdistclean: clean distclean-subdirs

mostlyclean: clean
realclean: distclean
clobber: distclean

# Semi-automatic generation of dependencies:
# Use gcc if possible because X11 `makedepend' doesn't work on all systems
# and it also includes system headers.
# Also test for the presence of source files, if not do nothing.
depend:
ifeq ("$(wildcard *.c)","")
	@$(ECHO) "Nothing to do for depend."
else
	@$(RMF) Makefile.bak
	@if test "$(CC)" = "gcc" ; then \
	   $(MV) Makefile Makefile.bak; \
	   $(SED) "/^# DO NOT DELETE THIS LINE/,$$ d" Makefile.bak > Makefile; \
	   $(ECHO) "# DO NOT DELETE THIS LINE -- make depend depends on it." >> Makefile; \
	   $(CC) -S -M $(CPPFLAGS) $(INCFLAGS) *.c >> Makefile; \
	else \
	   makedepend -- $(CFLAGS) -- $(INCFLAGS) *.c; \
	fi 
	@if test -f Makefile ; then \
	    $(RMF) Makefile.bak; \
	else \
	   $(MV) Makefile.bak Makefile; \
	   echo -e "Something went wrong with the make depend!\n\a\a\a\a"; \
	fi
endif
	@if test ! -z "$(subdirs)"; then \
		for file in . ${subdirs}; \
		do \
			(cd $$file && if test "$$file" != "."; then make depend; fi); \
		done; \
	fi

install-subdirs:
	@if test ! -z "$(subdirs)"; then \
		for file in . ${subdirs}; \
		do \
			(cd $$file && if test "$$file" != "."; then make install; fi); \
		done; \
	fi

uninstall-subdirs:
	@if test ! -z "$(subdirs)"; then \
		for file in . ${subdirs}; \
		do \
			(cd $$file && if test "$$file" != "."; then make uninstall; fi); \
		done; \
	fi

indent:
	(cd $(topdir) && \
		find . \( -name '*.c' -o -name '*.h' -o -name '*.cpp' \) \
			-exec ./scripts/format_code {} \;)

TAGS:
	(cd $(topdir) && $(ETAGS) `find . \( -name \*.c -o -name \*.h \)`)
tags:
	(cd $(topdir) && $(CTAGS) `find . \( -name \*.c -o -name \*.h \)`)
