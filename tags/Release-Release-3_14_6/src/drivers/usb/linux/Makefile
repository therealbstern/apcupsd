topdir:=../../../..
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.c)

all-targets: $(OBJS)

# Include dependencies
-include $(SRCS:%.c=$(DEPDIR)/%.P)
