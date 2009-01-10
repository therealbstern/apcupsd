topdir:=../..
include $(topdir)/autoconf/targets.mak

SRCS = apcconfig.c apcerror.c apcevents.c apcexec.c \
       apcfile.c apclibnis.c apclock.c apclog.c apcsignal.c \
       apcstatus.c asys.c newups.c md5.c $(LIBEXTRAOBJ:.o=.c)

all-targets: libapc.a

libapc.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(SRCS:%.c=$(DEPDIR)/%.P)
