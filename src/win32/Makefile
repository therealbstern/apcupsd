topdir:=../..

SUBDIRS = compat

include $(topdir)/autoconf/targets.mak

# Objects needed by apcupsd daemon
SRCS := winmain.cpp winservice.cpp

# Objects needed by various windows-specific executables
APCTRAY_SRC := apctray.cpp winabout.cpp winevents.cpp winstat.cpp wintray.cpp \
   balloonmgr.cpp meter.cpp listview.cpp instmgr.cpp winconfig.cpp winres.res
SHUTDOWN_SRC := shutdown.c
POPUP_SRC := popup.c
EMAIL_SRC := email.c 
BG_SRC := background.c

apctray$(EXE): LIBS += -lrpcrt4
email$(EXE): LIBS += -lmapi32

all-targets: $(OBJS) apctray$(EXE) shutdown$(EXE) popup$(EXE) email$(EXE) background$(EXE)

apctray$(EXE): $(call SRC2OBJ, $(APCTRAY_SRC)) $(APCLIBS)
	$(LINK) $(DRVLIBS) $(BG)

shutdown$(EXE): $(call SRC2OBJ, $(SHUTDOWN_SRC))
	$(LINK) $(BG)

popup$(EXE): $(call SRC2OBJ, $(POPUP_SRC))
	$(LINK) $(BG)

email$(EXE): $(call SRC2OBJ, $(EMAIL_SRC)) $(APCLIBS)
	$(LINK)

background$(EXE): $(call SRC2OBJ, $(BG_SRC))
	$(LINK) $(BG)

# Include dependencies
-include $(DEPS)
