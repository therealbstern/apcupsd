topdir:=../../..

SUBDIRS = 

include $(topdir)/autoconf/targets.mak

SRCS = compat.cpp inet_aton.c winapi.c $(if $(USBDRV),libusb-winusb-bridge.c) \
   ioctl.c tcflush.c tcsetattr.c tcgetattr.c syslog.c kill.c pathconf.c \
   waitpid.c sleep.c fcntl.c

all-targets: libwin32compat.a

libwin32compat.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
