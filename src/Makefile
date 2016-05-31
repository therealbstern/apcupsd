topdir:=..

SUBDIRS = lib drivers $(CGIDIR) $(USBHIDDIR) $(GAPCMON) $(APCAGENT) $(WIN32)
include $(topdir)/autoconf/targets.mak

# Force dirs that needs libapc.a to build after lib_DIR
# Necessary for parallel make since they link against libapc.a
cgi_DIR: lib_DIR
apcagent_DIR: lib_DIR
win32_DIR: lib_DIR

# CGI requires win32, but only if building for win32
cgi_DIR: $(if $(WIN32),win32_DIR)

common_srcs     := options.c device.c reports.c action.c
apcupsd_srcs    := apcupsd.c apcnis.c
apcaccess_srcs  := apcaccess.c
apctest_srcs    := apctest.c
smtp_srcs       := smtp.c

common_obj      := $(call SRC2OBJ,$(common_srcs))
apcupsd_obj     := $(call SRC2OBJ,$(apcupsd_srcs)) $(if $(WIN32),$(call SRC2OBJ,win32/winservice.cpp win32/winmain.cpp))
apcaccess_obj   := $(call SRC2OBJ,$(apcaccess_srcs))
apctest_obj     := $(call SRC2OBJ,$(apctest_srcs))
smtp_obj        := $(call SRC2OBJ,$(smtp_srcs))

SRCS = $(common_srcs) $(apcupsd_srcs) $(apcaccess_srcs) \
       $(apctest_srcs) $(smtp_srcs) 

all-targets: apcupsd$(EXE) apcaccess$(EXE) apctest$(EXE) smtp$(EXE) 

apcupsd$(EXE): $(common_obj) $(apcupsd_obj) $(APCDRVLIBS) $(APCLIBS)
	$(LINK) $(DRVLIBS) $(BG)

apctest$(EXE): $(common_obj) $(apctest_obj) $(APCDRVLIBS) $(APCLIBS)
	$(LINK) $(DRVLIBS)

apcaccess$(EXE): $(apcaccess_obj) $(APCLIBS)
	$(LINK)

smtp$(EXE): $(smtp_obj) $(APCLIBS)
	$(LINK)

all-install: install-base
all-uninstall: uninstall-base

install-base:
	$(call MKDIR,$(sbindir))
	$(call INSTPROG,700,apcupsd,$(sbindir)/apcupsd$(EXE))
	$(call INSTPROG,700,apctest,$(sbindir)/apctest$(EXE))
	$(call INSTPROG,755,apcaccess,$(sbindir)/apcaccess$(EXE))
	$(call INSTPROG,755,smtp,$(sbindir)/smtp$(EXE))

uninstall-base:
	$(call UNINST,$(sbindir)/apcupsd$(EXE))
	$(call UNINST,$(sbindir)/apctest$(EXE))
	$(call UNINST,$(sbindir)/apcaccess$(EXE))
	$(call UNINST,$(sbindir)/smtp$(EXE))

# Include dependencies
-include $(DEPS)
