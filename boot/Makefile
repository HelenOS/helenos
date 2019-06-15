#
# Copyright (c) 2006 Martin Decky
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

.PHONY: all build_dist clean_dist clean

include Makefile.common

all: $(VERSION_DEF) $(COMMON_MAKEFILE) $(CONFIG_MAKEFILE) $(CONFIG_HEADER) $(PREBUILD) build_dist
	$(MAKE) -r -f $(BUILD) PRECHECK=$(PRECHECK)
ifneq ($(POSTBUILD),)
	$(MAKE) -r -f $(POSTBUILD) PRECHECK=$(PRECHECK)
endif

build_dist: clean_dist
	cp -r -L -T -u "$(USPACE_PATH)/dist" "$(DIST_PATH)"
	mkdir -p "$(DIST_PATH)/app/"
	mkdir -p "$(DIST_PATH)/cfg/"
	mkdir -p "$(DIST_PATH)/data/"
	mkdir -p "$(DIST_PATH)/lib/"
	mkdir -p "$(DIST_PATH)/loc/"
	mkdir -p "$(DIST_PATH)/log/"
	mkdir -p "$(DIST_PATH)/test/"
	mkdir -p "$(DIST_PATH)/tmp/"
	mkdir -p "$(DIST_PATH)/vol/"
	mkdir -p "$(DIST_PATH)/w/"

ifeq ($(CONFIG_DEVEL_FILES), y)
	cp -L "$(LIBGCC_PATH)" "$(DIST_PATH)/lib/"
endif

	if ls $(DIST_OVERLAY_PATH)/* >/dev/null 2>/dev/null; then \
		cp -r -L $(DIST_OVERLAY_PATH)/* "$(DIST_PATH)"; \
	fi

clean: clean_dist
	$(MAKE) -r -f $(BUILD) clean PRECHECK=$(PRECHECK)
ifneq ($(POSTBUILD),)
	$(MAKE) -r -f $(POSTBUILD) clean PRECHECK=$(PRECHECK)
endif
	rm -f $(POST_OUTPUT) $(BOOT_OUTPUT) arch/*/include/common.h
	find generic/src/ arch/*/src/ genarch/src/ -name '*.o' -follow -exec rm \{\} \;
	find . -name '*.d' -follow -exec rm \{\} \;

clean_dist:
	rm -f $(INITRD).img $(COMPS).o $(COMPS).tar $(LINK)
	find $(USPACE_PATH)/dist -mindepth 1 -maxdepth 1 -type f -exec rm \{\} \;
	rm -rf $(USPACE_PATH)/dist/app
	rm -f $(USPACE_PATH)/dist/cfg/*
	rm -rf $(USPACE_PATH)/dist/srv
	rm -rf $(USPACE_PATH)/dist/drv/*
	rm -rf $(USPACE_PATH)/dist/lib/*
	rm -rf $(USPACE_PATH)/dist/inc/*
	rm -f $(USPACE_PATH)/dist/app/*
	rm -f $(USPACE_PATH)/dist/test/*
	rm -f $(USPACE_PATH)/dist/cfg/net/*
	rm -f $(USPACE_PATH)/dist/w/*
