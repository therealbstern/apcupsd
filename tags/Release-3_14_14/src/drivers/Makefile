topdir:=../..

SUBDIRS = $(SMARTDRV) $(DUMBDRV) $(NETDRV) $(PCNETDRV) $(USBDRV) \
          $(SNMPLTDRV) $(TESTDRV) $(MODBUSDRV)

include $(topdir)/autoconf/targets.mak

SRCS = drivers.c

all-targets: libdrivers.a

libdrivers.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
