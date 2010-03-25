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

-include ../Makefile.common
-include ../Makefile.config

## Common binaries
#

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
	app/ps \
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
	srv/hid/adb_mouse \
	srv/hid/char_mouse \
	srv/hid/fb \
	srv/hid/kbd \
	srv/hw/char/i8042

## Platform-specific hardware support
#

ifneq ($(UARCH),abs32le)
	DIRS += srv/hid/console
endif

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

## Networking
#

ifeq ($(CONFIG_NETIF_DP8390),y)
	DIRS += srv/net/netif/dp8390
endif

NETWORKING_COMMON = \
	srv/net/netif/lo \
	srv/net/nil/eth \
	srv/net/nil/nildummy \
	srv/net/net \
	srv/net/net/start \
	srv/net/socket \
	srv/net/app/echo \
	srv/net/app/ping \
	srv/net/app/nettest1 \
	srv/net/app/nettest2

NETWORKING_MODULAR = \
	srv/net/il/arp \
	srv/net/il/ip \
	srv/net/tl/icmp \
	srv/net/tl/udp \
	srv/net/tl/tcp

## System libraries
#

LIBC = lib/c
LIBS = \
	lib/fs \
	lib/block \
	lib/softint \
	lib/softfloat \

ifeq ($(UARCH),amd64)
	LIBS += lib/pci
endif

ifeq ($(UARCH),ia32)
	LIBS += lib/pci
endif


LIBC_BUILD = $(addsuffix .build,$(LIBC))
LIBS_BUILD = $(addsuffix .build,$(LIBS))

ifeq ($(CONFIG_NETWORKING),modular)
	BUILDS := $(addsuffix .build,$(DIRS)) $(addsuffix .build,$(NETWORKING_COMMON)) $(addsuffix .build,$(NETWORKING_MODULAR))
endif

ifeq ($(CONFIG_NETWORKING),module)
	BUILDS := $(addsuffix .build,$(DIRS)) $(addsuffix .build,$(NETWORKING_COMMON))
endif

ifeq ($(CONFIG_NETWORKING),none)
	BUILDS := $(addsuffix .build,$(DIRS))
endif

CLEANS := $(addsuffix .clean,$(DIRS)) $(addsuffix .clean,$(NETWORKING_COMMON)) $(addsuffix .clean,$(NETWORKING_MODULAR)) $(addsuffix .clean,$(LIBS)) $(addsuffix .clean,$(LIBC))

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
