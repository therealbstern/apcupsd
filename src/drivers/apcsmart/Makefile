topdir:=../../..
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.c)

all-targets: libapcsmartdrv.a

libapcsmartdrv.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
