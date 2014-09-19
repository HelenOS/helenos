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

.PHONY: all build_dist build_comps clean_dist clean

include Makefile.common

all: $(VERSION_DEF) $(COMMON_MAKEFILE) $(COMMON_HEADER) $(CONFIG_MAKEFILE) $(CONFIG_HEADER) $(PREBUILD)
	$(MAKE) -f $(BUILD) PRECHECK=$(PRECHECK)
ifneq ($(POSTBUILD),)
	$(MAKE) -f $(POSTBUILD) PRECHECK=$(PRECHECK)
endif

build_comps: $(COMPONENTS) $(LINK).in
	$(PACK) $(OBJCOPY) $(BFD_NAME) $(BFD_ARCH) arch/$(KARCH) $(COMPONENTS)

$(INITRD).img: build_dist
ifeq ($(RDFMT),tmpfs)
	$(MKTMPFS) $(DIST_PATH) $@
endif
ifeq ($(RDFMT),fat)
	$(MKFAT) 1048576 $(DIST_PATH) $@
endif
ifeq ($(RDFMT),ext4fs)
	$(MKEXT4) 1048576 $(DIST_PATH) $@
endif

build_dist: clean_dist
	for file in $(RD_SRVS) ; do \
		cp "$$file" "$(DIST_PATH)/srv/" ; \
	done
	for file in $(RD_LIBS) ; do \
		cp "$$file" "$(DIST_PATH)/lib/" ; \
	done
ifeq ($(CONFIG_DEVEL_FILES), y)
	mkdir "$(DIST_PATH)/inc/c/"
	cp -r -L "$(USPACE_PATH)/lib/c/include/." "$(DIST_PATH)/inc/c/"
	cp -r -L "$(ROOT_PATH)/abi/include/." "$(DIST_PATH)/inc/c/"
	cp -r -L "$(USPACE_PATH)/lib/c/arch/$(UARCH)/include/." "$(DIST_PATH)/inc/c/"
	cat "$(USPACE_PATH)/lib/c/arch/$(UARCH)/_link.ld" | sed 's/^STARTUP(.*)$$//g' > "$(DIST_PATH)/inc/_link.ld"
endif
	for file in $(RD_APPS) ; do \
		cp "$$file" "$(DIST_PATH)/app/" ; \
	done
ifeq ($(CONFIG_PCUT_TESTS),y)
	echo "echo Running all tests..." >"$(DIST_PATH)/test/run_all"
	echo "<html><head><title>HelenOS test results</title></head><body>" >"$(DIST_PATH)/test/test.html"
	echo "<h1>HelenOS test results</h1><ul>" >>"$(DIST_PATH)/test/test.html"
	for file in $(RD_TESTS) ; do \
		file2=`basename $$file`; \
		cp "$$file" "$(DIST_PATH)/test/" ; \
		echo "echo ' ->' $$file2" >>"$(DIST_PATH)/test/run_all"; \
		echo "/test/$$file2 | to /test/$$file2.out" >>"$(DIST_PATH)/test/run_all"; \
		echo "cat /test/$$file2.out" >>"$(DIST_PATH)/test/run_all"; \
		echo "cp -f /test/$$file2.out /data/web/result-$$file2.txt" >>"$(DIST_PATH)/test/run_all"; \
		echo "<li><a href=\"result-$$file2.txt\">$$file2</a></li>" >>"$(DIST_PATH)/test/test.html"; \
	done
	echo "cp -f /test/test.html /data/web/test.html" >>"$(DIST_PATH)/test/run_all"
	echo "</ul></body></html>" >>"$(DIST_PATH)/test/test.html"
endif

ifeq ($(CONFIG_PCUT_SELF_TESTS),y)
	echo "echo Running all PCUT self-tests..." >"$(DIST_PATH)/test/run_pcut"
	echo "<html><head><title>Results of PCUT self-tests on HelenOS</title></head><body>" >"$(DIST_PATH)/test/pcut.html"
	echo "<h1>Results of PCUT self-tests on HelenOS</h1><ul>" >>"$(DIST_PATH)/test/pcut.html"
	for file in $(USPACE_PATH)/lib/pcut/test-libpcut-*; do \
		file2=`basename $$file`; \
		name=`echo "$$file2" | sed 's/test-libpcut-//'`; \
		cp "$$file" "$(DIST_PATH)/test/" ; \
		echo "echo ' ->' $$name" >>"$(DIST_PATH)/test/run_pcut"; \
		echo "/test/$$file2 | to /test/$$file2.out" >>"$(DIST_PATH)/test/run_pcut"; \
		echo "cat /test/$$file2.out" >>"$(DIST_PATH)/test/run_pcut"; \
		echo "cp -f /test/$$file2.out /data/web/result-$$file2.txt" >>"$(DIST_PATH)/test/run_pcut"; \
		echo "<li><a href=\"result-$$file2.txt\">$$name</a></li>" >>"$(DIST_PATH)/test/pcut.html"; \
	done
	echo "cp -f /test/pcut.html /data/web/pcut.html" >>"$(DIST_PATH)/test/run_pcut"
	echo "</ul></body></html>" >>"$(DIST_PATH)/test/pcut.html"
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
		cp "$(USPACE_PATH)/$(DRVS_PATH)/$$file_dir/$$file_name/$$file_name.dev" "$(DIST_PATH)/$(DRVS_PATH)/$$file_name/" ; \
	done
	if ls $(DIST_OVERLAY_PATH)/* >/dev/null; then \
		cp -r -L $(DIST_OVERLAY_PATH)/* "$(DIST_PATH)"; \
	fi

clean: clean_dist
	$(MAKE) -f $(BUILD) clean PRECHECK=$(PRECHECK)
ifneq ($(POSTBUILD),)
	$(MAKE) -f $(POSTBUILD) clean PRECHECK=$(PRECHECK)
endif
	rm -fr $(SANDBOX)
	rm -f $(POST_OUTPUT) $(BOOT_OUTPUT) $(DEPEND) $(DEPEND_PREV) arch/*/include/common.h
	find generic/src/ arch/*/src/ genarch/src/ -name '*.o' -follow -exec rm \{\} \;

clean_dist:
	rm -f $(INITRD).img $(COMPS_H) $(COMPS_C) $(LINK) $(LINK).comp *.co
	rm -f $(USPACE_PATH)/dist/srv/*
	rm -rf $(USPACE_PATH)/dist/drv/*
	rm -rf $(USPACE_PATH)/dist/lib/*
	rm -rf $(USPACE_PATH)/dist/inc/*
	rm -f $(USPACE_PATH)/dist/app/*
	rm -f $(USPACE_PATH)/dist/test/*
	rm -f $(USPACE_PATH)/dist/cfg/net/*
