topdir:=../..
include $(topdir)/autoconf/targets.mak

SRCS = apcconfig.c apcerror.c apcevents.c apcexec.c \
       apcfile.c apclibnis.c apclock.c apclog.c apcsignal.c \
       astring.cpp amutex.cpp atimer.cpp athread.cpp autil.cpp \
       smartcmd.cpp apcstatus.c asys.c newups.c md5.c \
       $(LIBEXTRAOBJ:.o=.c)

all-targets: libapc.a

libapc.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
