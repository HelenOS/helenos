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

all: $(VERSION_DEF) $(COMMON_MAKEFILE) $(COMMON_HEADER) $(CONFIG_MAKEFILE) $(CONFIG_HEADER) $(PREBUILD) build_dist
	$(MAKE) -r -f $(BUILD) PRECHECK=$(PRECHECK)
ifneq ($(POSTBUILD),)
	$(MAKE) -r -f $(POSTBUILD) PRECHECK=$(PRECHECK)
endif

build_dist: clean_dist
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

	for srv in $(RD_SRVS) ; do \
		srv_dir="`dirname "$$srv"`" ; \
		srv_name="`basename "$$srv"`" ; \
		mkdir -p "$(DIST_PATH)/srv/$$srv_dir" ; \
		cp "$(USPACE_PATH)/srv/$$srv/$$srv_name" "$(DIST_PATH)/srv/$$srv" ; \
	done

	for file in $(RD_LIBS) ; do \
		cp "$$file" "$(DIST_PATH)/lib/" ; \
	done

ifeq ($(CONFIG_DEVEL_FILES), y)
	mkdir -p "$(DIST_PATH)/inc/c/"
	cp -r -L "$(USPACE_PATH)/lib/c/include/." "$(DIST_PATH)/inc/c/"
	cp -r -L "$(ROOT_PATH)/abi/include/." "$(DIST_PATH)/inc/c/"
	cp -r -L "$(USPACE_PATH)/lib/c/arch/$(UARCH)/include/." "$(DIST_PATH)/inc/c/"
	cp -L "$(USPACE_PATH)/lib/c/crt0.o" "$(DIST_PATH)/lib/"
	cp -L "$(USPACE_PATH)/lib/c/crt1.o" "$(DIST_PATH)/lib/"
	cp -L "$(LIBGCC_PATH)" "$(DIST_PATH)/lib/"
endif

	for app in $(RD_APPS) ; do \
		app_dir="`dirname "$$app"`" ; \
		app_name="`basename "$$app"`" ; \
		mkdir -p "$(DIST_PATH)/app/$$app_dir" ; \
		cp "$(USPACE_PATH)/app/$$app/$$app_name" "$(DIST_PATH)/app/$$app" ; \
	done
	for file in $(RD_DATA) ; do \
		cp "$$file" "$(DIST_PATH)/" ; \
	done

ifeq ($(CONFIG_PCUT_TESTS),y)
	for file in $(RD_TESTS) ; do \
		cp "$$file" "$(DIST_PATH)/test/" ; \
	done
endif

ifeq ($(CONFIG_PCUT_SELF_TESTS),y)
	for file in $(USPACE_PATH)/lib/pcut/test-libpcut-*; do \
		cp "$$file" "$(DIST_PATH)/test/" ; \
	done
endif

	for drv in $(RD_DRVS) ; do \
		drv_dir="`dirname "$$drv"`" ; \
		drv_name="`basename "$$drv"`" ; \
		mkdir -p "$(DIST_PATH)/$(DRVS_PATH)/$$drv_name" ; \
		cp "$(USPACE_PATH)/$(DRVS_PATH)/$$drv_dir/$$drv_name/$$drv_name" "$(DIST_PATH)/$(DRVS_PATH)/$$drv_name/" ; \
		cp "$(USPACE_PATH)/$(DRVS_PATH)/$$drv_dir/$$drv_name/$$drv_name.ma" "$(DIST_PATH)/$(DRVS_PATH)/$$drv_name/" ; \
	done
	for file in $(RD_DRV_CFG) ; do \
		file_dir="`dirname "$$file"`" ; \
		file_name="`basename "$$file"`" ; \
		cp "$(USPACE_PATH)/$(DRVS_PATH)/$$file_dir/$$file_name/"*".dev" "$(DIST_PATH)/$(DRVS_PATH)/$$file_name/" ; \
	done
	for file in $(RD_DRVS_FW) ; do \
		file_dir="`dirname "$$file"`" ; \
		file_name="`basename "$$file"`" ; \
		cp "$(USPACE_PATH)/$(DRVS_PATH)/$$file_dir/$$file_name/$$file_name.fw" "$(DIST_PATH)/$(DRVS_PATH)/$$file_name/" ; \
	done
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
