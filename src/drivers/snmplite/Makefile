topdir:=../../..
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.cpp) $(wildcard *.c)

all-targets: libsnmplitedrv.a

libsnmplitedrv.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
