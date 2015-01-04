topdir:=../../..
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.c)

all-targets: libpcnetdrv.a

libpcnetdrv.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
