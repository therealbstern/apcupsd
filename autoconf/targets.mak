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

# standard install target: Same logic as 'all'.
.PHONY: install
install: all-subdirs
	$(VV)+$(MAKE) $(NPD) all-targets
	$(VV)+$(MAKE) $(NPD) all-install

# 'all-install' is extended by lower-level Makefile to perform any
# required install actions.
.PHONY: all-install
all-install:
	@#

# no-op targets for use by lower-level Makefiles when a particular
# component is not being installed.
.PHONY: install- uninstall-
install-:
uninstall-:

# standard uninstall target: Depends on subdirs to force recursion,
# then reinvokes make to uninstall local targets. Same logic as 'install'.
.PHONY: uninstall
uninstall: all-subdirs
	$(VV)+$(MAKE) $(NPD) all-uninstall

# 'all-uninstall' is extended by lower-level Makefiles to perform 
# any required uninstall actions.
.PHONY: all-uninstall
all-uninstall:
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

# Rule to create a directory during install
define MKDIR
   $(if $(wildcard $(DESTDIR)$(1)),, \
      @echo "  MKDIR" $(DESTDIR)$(1))
   $(if $(wildcard $(DESTDIR)$(1)),, \
     $(V)$(MKINSTALLDIRS) $(DESTDIR)$(1))
endef

# Install a program file, given mode, src, and dest
define INSTPROG
   @echo "  COPY " $(2) =\> $(DESTDIR)$(3)
   $(V)$(INSTALL_PROGRAM) $(STRIP) -m $(1) $(2) $(DESTDIR)$(3)
endef

# Install a data file, given mode, src, and dest
define INSTDATA
   @echo "  COPY " $(2) =\> $(DESTDIR)$(3)
   $(V)$(INSTALL_DATA) -m $(1) $(2) $(DESTDIR)$(3)
endef

# Install a data file, given mode, src, and dest.
# Existing dest file is preserved; new file is named *.new if dest exists.
define INSTNEW
   @echo "  COPY " $(notdir $(2)) =\> $(DESTDIR)$(3)/$(notdir $(2))$(if $(wildcard $(DESTDIR)$(3)/$(notdir $(2))),.new,)
   $(V)$(INSTALL_DATA) -m $(1) $(2) $(DESTDIR)$(3)/$(notdir $(2))$(if $(wildcard $(DESTDIR)$(3)/$(notdir $(2))),.new,)
endef

# Install a data file, given mode, src, and dest.
# Existing dest file is renamed to *.orig if it exists.
define INSTORIG
   $(if $(wildcard $(DESTDIR)$(3)/$(notdir $(2))), \
      @echo "  MV   " $(DESTDIR)$(3)/$(notdir $(2)) =\> \
         $(DESTDIR)$(3)/$(notdir $(2)).orig,)
   $(if $(wildcard $(DESTDIR)$(3)/$(notdir $(2))), \
      $(V)mv $(DESTDIR)$(3)/$(notdir $(2)) $(DESTDIR)$(3)/$(notdir $(2)).orig,)
   @echo "  COPY " $(notdir $(2)) =\> $(DESTDIR)$(3)/$(notdir $(2))
   $(V)$(INSTALL_SCRIPT) -m $(1) $(2) $(DESTDIR)$(3)
endef

# Make a symlink
define SYMLINK
   @echo "  LN   " $(DESTDIR)/$(2) -\> $(1)
   $(V)ln -sf $(1) $(DESTDIR)/$(2)
endef

# Copy a file
define COPY
   @echo "  CP   " $(1) =\> $(DESTDIR)/$(2)
   $(V)cp -f $(1) $(DESTDIR)/$(2)
endef

# Uninstall a file
define UNINST
   @echo "  RM   " $(DESTDIR)$(1)
   $(V)$(RMF) $(DESTDIR)$(1)
endef

# Announce distro install
define DISTINST
   @echo "  ------------------------------------------------------------"
   @echo "  $(1) distribution installation"
   @echo "  ------------------------------------------------------------"
endef

# Announce distro uninstall
define DISTUNINST
   @echo "  ------------------------------------------------------------"
   @echo "  $(1) distribution uninstall"
   @echo "  ------------------------------------------------------------"
endef
