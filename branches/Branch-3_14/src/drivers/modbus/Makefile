topdir:=../../..
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.cpp)

all-targets: libmodbusdrv.a

libmodbusdrv.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
