topdir:=../../..
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.c)

all-targets: libdumbdrv.a

libdumbdrv.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
