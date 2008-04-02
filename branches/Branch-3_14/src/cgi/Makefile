topdir:=../..
include $(topdir)/autoconf/targets.mak

common_srcs    := upsfetch.c cgilib.c
multimon_srcs  := multimon.c
upsstats_srcs  := upsstats.c
upsfstats_srcs := upsfstats.c
upsimage_srcs  := upsimage.c

common_obj     := $(call SRC2OBJ,$(common_srcs))
multimon_obj   := $(call SRC2OBJ,$(multimon_srcs))
upsstats_obj   := $(call SRC2OBJ,$(upsstats_srcs))
upsfstats_obj  := $(call SRC2OBJ,$(upsfstats_srcs))
upsimage_obj   := $(call SRC2OBJ,$(upsimage_srcs))

SRCS = $(common_srcs) $(multimon_srcs) $(upsstats_srcs) \
       $(upsfstats_srcs) $(upsimage_srcs)

all-targets: multimon.cgi upsstats.cgi upsfstats.cgi upsimage.cgi

multimon.cgi: $(common_obj) $(multimon_obj) $(APCLIBS)
	$(LINK)

upsstats.cgi: $(common_obj) $(upsstats_obj) $(APCLIBS)
	$(LINK)

upsfstats.cgi: $(common_obj) $(upsfstats_obj) $(APCLIBS)
	$(LINK)

upsimage.cgi: $(common_obj) $(upsimage_obj) $(APCLIBS)
	$(LINK) $(LIBGD)

# Include dependencies
-include $(SRCS:%.c=$(DEPDIR)/%.P)
