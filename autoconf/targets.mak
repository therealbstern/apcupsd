# Pull in autoconf variables
include $(topdir)/autoconf/variables.mak

# Now that we have autoconf vars, overwrite $(topdir) with absolute path 
# version instead of relative version we inherited. The easy way to do this 
# would be to use $(abspath $(topdir)), but abspath is a gmake-3.81 feature.
# So we let autoconf figure it out for us.
topdir := $(abstopdir)

# Older (pre-3.79) gmake does not have $(CURDIR)
ifeq ($(CURDIR),)
  CURDIR := $(shell pwd)
endif

# By default we do pretty-printing only
V := @
VV := @
NPD := --no-print-directory

# Check verbose flag
ifeq ($(strip $(VERBOSE)),1)
  V:=
  NPD :=
endif
ifeq ($(strip $(VERBOSE)),2)
  V:=
  VV:=
  NPD :=
endif

# Relative path to this dir from $(topdir)
RELDIR := $(patsubst /%,%,$(subst $(topdir),,$(CURDIR)))
ifneq ($(strip $(RELDIR)),)
  RELDIR := $(RELDIR)/
endif

# Convert a list of sources to a list of objects in OBJDIR
SRC2OBJ = $(foreach obj,$(1:.c=.o),$(dir $(obj))$(OBJDIR)/$(notdir $(obj)))

# All objects, derived from all sources
OBJS = $(call SRC2OBJ,$(SRCS))

# Default target: Build all subdirs, then reinvoke make to build local 
# targets. This is a little gross, but necessary to force make to build 
# subdirs first when running in parallel via 'make -jN'. Hopefully I will 
# discover a cleaner way to solve this someday.
.PHONY: all
all: all-subdirs
	$(VV)+$(MAKE) $(NPD) all-targets

# 'all-targets' is supplied by lower level Makefile. It represents
# all targets to be built at that level. We list it here with a do-nothing
# action in order to suppress the "Nothing to do for all-targets" message
# when all targets are up to date.
.PHONY: all-targets
all-targets:
	@#

# Typical clean target: Remove all objects and dependency files.
.PHONY: clean
clean:
	$(V)find . -depth \
	  \( -name $(OBJDIR) -o -name $(DEPDIR) -o -name \*.a \) \
          -exec echo "  CLEAN" \{\} \; -exec rm -r \{\} \;

# Template rule to build a subdirectory
.PHONY: %_DIR
%_DIR:
	@echo "       " $(RELDIR)$*
	$(VV)+$(MAKE) -C $* $(NPD) $(MAKECMDGOALS)

# Collective all-subdirs target depends on subdir rule
.PHONY: all-subdirs
all-subdirs: $(foreach subdir,$(SUBDIRS),$(subdir)_DIR)

# How to build dependencies
MAKEDEPEND = $(CC) -M $(CFLAGS) $< > $(df).d
ifeq ($(strip $(NODEPS)),)
  define DEPENDS
	if test ! -d $(DEPDIR); then mkdir -p $(DEPDIR); fi; \
	  $(MAKEDEPEND); \
	  echo -n $(OBJDIR)/ > $(df).P; \
	  sed -e 's/#.*//' -e '/^$$/ d' < $(df).d >> $(df).P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $(df).d >> $(df).P; \
	  rm -f $(df).d
  endef
else
  DEPENDS :=
endif

# Rule to build *.o from *.c and generate dependencies for it
$(OBJDIR)/%.o: %.c
	@echo "  CXX  " $(RELDIR)$<
	$(VV)if test ! -d $(OBJDIR); then mkdir -p $(OBJDIR); fi
	$(V)$(CXX) $(CPPFLAGS) -c -o $@ $<
	$(VV)$(DEPENDS)

# Rule to link an executable
define LINK
	@echo "  LD   " $(RELDIR)$@
	$(V)$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)
endef

# Rule to generate an archive (library)
MAKELIB=$(call ARCHIVE,$@,$(OBJS))
define ARCHIVE
	@echo "  AR   " $(RELDIR)$(1)
	$(VV)rm -f $(1)
	$(V)$(AR) rc $(1) $(2)
	$(V)$(RANLIB) $(1)
endef
