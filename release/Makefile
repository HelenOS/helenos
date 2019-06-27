#
# Copyright (c) 2011 Jiri Svoboda
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

-include ../version

EXPORT_DIR = /var/tmp/helenos/HelenOS-$(RELEASE)
SRC_ARCHIVE = HelenOS-$(RELEASE).tar.bz2

PROFILES = \
	amd64 \
	arm32/beagleboardxm \
	arm32/beaglebone \
	arm32/gta02 \
	arm32/integratorcp \
	arm32/raspberrypi \
	arm64/virt \
	ia32 \
	ia64/i460GX \
	ia64/ski \
	mips32/malta-be \
	mips32/malta-le \
	mips32/msim \
	ppc32 \
	sparc64/niagara \
	sparc64/ultra

GIT = git
TAR = tar

# To determine IMG_FILE
-include $(EXPORT_DIR)/Makefile.config
-include $(EXPORT_DIR)/boot/arch/$(BARCH)/Makefile.inc
-include $(EXPORT_DIR)/boot/Makefile.common

ifdef POST_OUTPUT
	IMG_FILE = $(EXPORT_DIR)/boot/$(POST_OUTPUT)
else
	IMG_FILE = $(EXPORT_DIR)/boot/$(BOOT_OUTPUT)
endif

SUFFIX = $(suffix $(IMG_FILE))

ifdef PROFILE
	RELEASE_FILE = HelenOS-$(RELEASE)-$(shell echo $(PROFILE) | tr '/' '-')$(SUFFIX)
else
	RELEASE_FILE = HelenOS-$(RELEASE)-$(PLATFORM)-$(MACHINE)-$(PROCESSOR)$(SUFFIX)
endif

.PHONY: all clean release releasefile export src_rel

all: releasefile
releasefile: $(RELEASE_FILE)

$(RELEASE_FILE): $(IMG_FILE)
	cp $< $@

$(IMG_FILE):
	$(MAKE) -r -C $(EXPORT_DIR)

release: src_rel
	mkdir -p $(EXPORT_DIR)/uspace/overlay
	(cd ..; tools/cc.sh) >$(EXPORT_DIR)/uspace/overlay/COPYRIGHT
	for profile in $(PROFILES); do \
		$(MAKE) -r -C $(EXPORT_DIR) distclean ; \
		$(MAKE) -r -C $(EXPORT_DIR) PROFILE=$$profile ; \
		$(MAKE) -r PROFILE=$$profile releasefile ; \
	done

$(EXPORT_DIR) export:
	rm -rf $(EXPORT_DIR)
	mkdir -p $(EXPORT_DIR)
	(cd ..; $(GIT) archive master) | $(TAR) -x -C $(EXPORT_DIR)

src_rel $(SRC_ARCHIVE): export
	$(TAR) -C $(shell dirname $(EXPORT_DIR)) \
	    -cjf $(SRC_ARCHIVE) $(shell basename $(EXPORT_DIR))

clean:
	rm -f $(RELEASE_FILE)
