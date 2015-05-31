topdir:=../../..
SUBDIRS = $(USBTYPE)
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.c)

OBJS += $(call SRC2OBJ,$(wildcard $(USBTYPE)/*.c))
OBJS += $(call SRC2OBJ,$(wildcard $(USBTYPE)/*.cpp))

all-targets: libusbdrv.a

libusbdrv.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
