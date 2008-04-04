topdir:=../..
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.c)

# This bit of hackery is because gapcmon has its own CFLAGS and LIBS.
# Plus it needs to be built with a C compiler while normally all apcupsd *.c
# files are built with a C++ compiler. At some point we'll rename all *.c
# files in the rest of the tree to *.cpp and some of this can then go away.
CXX = $(CC)
CFLAGS += $(GAPCMON_CFLAGS)
CPPFLAGS = $(CFLAGS)
LIBS := $(GAPCMON_LIBS) $(X_LIBS) -lX11 -lXext $(LIBS)

all-targets: gapcmon

gapcmon: $(OBJS)
	$(LINK)

# Include dependencies
-include $(SRCS:%.c=$(DEPDIR)/%.P)
