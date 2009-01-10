topdir:=../..

SUBDIRS = $(SMARTDRV) $(DUMBDRV) $(NETDRV) $(PCNETDRV) $(USBDRV) \
          $(SNMPDRV) $(TESTDRV)

include $(topdir)/autoconf/targets.mak

# Append to OBJS the path to each driver object
OBJS += $(foreach subdir,$(SUBDIRS),$(call SRC2OBJ,$(wildcard $(subdir)/*.c)))
ifneq ($(strip $(USBDRV)),)
  OBJS += $(call SRC2OBJ,$(wildcard $(USBDRV)/$(USBTYPE)/*.c))
endif

SRCS = drivers.c

all-targets: libdrivers.a

libdrivers.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(SRCS:%.c=$(DEPDIR)/%.P)
