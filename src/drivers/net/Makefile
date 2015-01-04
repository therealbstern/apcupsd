topdir:=../../..
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.c)

all-targets: libnetdrv.a

libnetdrv.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
