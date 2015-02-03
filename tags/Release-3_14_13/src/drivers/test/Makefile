topdir:=../../..
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.c)

all-targets: libtestdrv.a

libtestdrv.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
