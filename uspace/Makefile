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

-include ../Makefile.config

DIRS = \
	app/bdsh \
	app/edit \
	app/getterm \
	app/init \
	app/klog \
	app/mkfat \
	app/redir \
	app/taskdump \
	app/tester \
	app/tetris \
	app/trace \
	srv/clip \
	srv/devmap \
	srv/loader \
	srv/ns \
	srv/taskmon \
	srv/vfs \
	srv/bd/ata_bd \
	srv/bd/file_bd \
	srv/bd/gxe_bd \
	srv/bd/rd \
	srv/bd/part/guid_part \
	srv/bd/part/mbr_part \
	srv/fs/fat \
	srv/fs/tmpfs \
	srv/fs/devfs \
	srv/hid/console \
	srv/hid/c_mouse \
	srv/hid/fb \
	srv/hid/kbd \
	srv/hw/char/i8042

ifeq ($(UARCH),amd64)
	DIRS += srv/hw/bus/pci
endif

ifeq ($(UARCH),ia32)
	DIRS += srv/hw/bus/pci
endif

ifeq ($(UARCH),ppc32)
	DIRS += srv/hw/bus/cuda_adb
endif

ifeq ($(UARCH),sparc64)
	DIRS += \
		srv/hw/cir/fhc \
		srv/hw/cir/obio
endif

LIBC = lib/libc
LIBS = \
	lib/libfs \
	lib/libblock \
	lib/softint \
	lib/softfloat \

ifeq ($(UARCH),amd64)
	LIBS += lib/libpci
endif

ifeq ($(UARCH),ia32)
	LIBS += lib/libpci
endif


LIBC_BUILD = $(addsuffix .build,$(LIBC))
LIBS_BUILD = $(addsuffix .build,$(LIBS))

BUILDS := $(addsuffix .build,$(DIRS))
CLEANS := $(addsuffix .clean,$(DIRS)) $(addsuffix .clean,$(LIBS)) $(addsuffix .clean,$(LIBC))

.PHONY: all $(LIBC_BUILD) $(LIBS_BUILD) $(BUILDS) $(CLEANS) clean

all: $(BUILDS)

clean: $(CLEANS)

$(CLEANS):
	-$(MAKE) -C $(basename $@) clean

$(BUILDS): $(LIBC_BUILD) $(LIBS_BUILD)
	$(MAKE) -C $(basename $@) all PRECHECK=$(PRECHECK)

$(LIBS_BUILD): $(LIBC_BUILD)
	$(MAKE) -C $(basename $@) all PRECHECK=$(PRECHECK)

$(LIBC_BUILD):
	$(MAKE) -C $(basename $@) all PRECHECK=$(PRECHECK)
