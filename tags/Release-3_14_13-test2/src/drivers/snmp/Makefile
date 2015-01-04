topdir:=../../..
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.c)

all-targets: libsnmpdrv.a

libsnmpdrv.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
