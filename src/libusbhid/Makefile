topdir:=../..
include $(topdir)/autoconf/targets.mak

SRCS = parse.c data.c descr.c HidUps.cpp

all-targets: libusbhid.a

libusbhid.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
