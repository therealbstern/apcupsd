topdir:=../../..

SUBDIRS = 

include $(topdir)/autoconf/targets.mak

SRCS = compat.cpp netcompat.c winapi.c $(if $(USBDRV),libusb-winusb-bridge.c)

all-targets: libwin32compat.a

libwin32compat.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
