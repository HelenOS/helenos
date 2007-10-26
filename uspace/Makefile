#
# Copyright (c) 2005 Martin Decky
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

## Include configuration
#

-include Makefile.config

DIRS = \
	lib/libc \
	lib/softint \
	lib/softfloat \
	srv/ns \
	srv/fb \
	srv/kbd \
	srv/console \
	srv/rd \
	srv/fs/fat \
	srv/vfs \
	srv/devmap \
	app/tetris \
	app/tester \
	app/klog \
	app/init

ifeq ($(ARCH), amd64)
	DIRS += srv/pci
endif

ifeq ($(ARCH), ia32)
	DIRS += srv/pci
endif

ifeq ($(ARCH), mips32)
	CFLAGS += -DCONFIG_MIPS_FPU
endif

ifeq ($(ARCH), mips32eb)
	CFLAGS += -DCONFIG_MIPS_FPU
endif

BUILDS := $(addsuffix .build,$(DIRS))
CLEANS := $(addsuffix .clean,$(DIRS))

.PHONY: all config build $(BUILDS) $(CLEANS) clean distclean

all:
	../tools/config.py uspace.config default $(ARCH) $(COMPILER) $(CONFIG_DEBUG)
	$(MAKE) -C . build

config:
	../tools/config.py uspace.config

build: $(BUILDS)

clean: $(CLEANS)
	find $(DIRS) -name '*.o' -follow -exec rm \{\} \;
	find lib/libc -name "_link.ld" -exec rm \{\} \;

distclean: clean
	-rm Makefile.config

$(CLEANS):
	-$(MAKE) -C $(basename $@) clean ARCH=$(ARCH)

$(BUILDS):
	$(MAKE) -C $(basename $@) all ARCH=$(ARCH) COMPILER=$(COMPILER)
