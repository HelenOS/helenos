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
	app/barber \
	app/bdsh \
	app/bithenge \
	app/blkdump \
	app/bnchmark \
	app/corecfg \
	app/devctl \
	app/dnscfg \
	app/dnsres \
	app/download \
	app/edit \
	app/fontviewer \
	app/getterm \
	app/hdisk \
	app/init \
	app/inet \
	app/kill \
	app/killall \
	app/kio \
	app/loc \
	app/logset \
	app/mixerctl \
	app/mkfat \
	app/mkexfat \
	app/mkmfs \
	app/modplay \
	app/nterm \
	app/redir \
	app/sbi \
	app/sportdmp \
	app/stats \
	app/taskdump \
	app/tester \
	app/testread \
	app/testwrit \
	app/tetris \
	app/trace \
	app/top \
	app/untar \
	app/usbinfo \
	app/vuhid \
	app/netecho \
	app/netspeed \
	app/nettest1 \
	app/nettest2 \
	app/nettest3 \
	app/nic \
	app/ping \
	app/sysinfo \
	app/mkbd \
	app/date \
	app/vdemo \
	app/viewer \
	app/vlaunch \
	app/vterm \
	app/df \
	app/wavplay \
	app/websrv \
	srv/audio/hound \
	srv/clipboard \
	srv/locsrv \
	srv/logger \
	srv/klog \
	srv/devman \
	srv/loader \
	srv/net/dhcp \
	srv/net/dnsrsrv \
	srv/net/ethip \
	srv/net/inetsrv \
	srv/net/loopip \
	srv/net/nconfsrv \
	srv/net/slip \
	srv/net/tcp \
	srv/net/udp \
	srv/ns \
	srv/taskmon \
	srv/vfs \
	srv/bd/sata_bd \
	srv/bd/file_bd \
	srv/bd/rd \
	srv/bd/part/guid_part \
	srv/bd/part/mbr_part \
	srv/fs/exfat \
	srv/fs/udf \
	srv/fs/fat \
	srv/fs/cdfs \
	srv/fs/tmpfs \
	srv/fs/mfs \
	srv/fs/locfs \
	srv/fs/ext4fs \
	srv/hid/compositor \
	srv/hid/console \
	srv/hid/s3c24xx_ts \
	srv/hid/isdv4_tablet \
	srv/hid/input \
	srv/hid/output \
	srv/hid/remcons \
	srv/hw/char/s3c24xx_uart \
	srv/hid/rfb \
	drv/audio/hdaudio \
	drv/audio/sb16 \
	drv/root/root \
	drv/root/virt \
	drv/block/ahci \
	drv/block/ata_bd \
	drv/char/i8042 \
	drv/char/pl050 \
	drv/char/ps2mouse \
	drv/char/xtkbd \
	drv/test/test1 \
	drv/test/test2 \
	drv/test/test3 \
	drv/fb/kfb \
	drv/bus/usb/ehci \
	drv/bus/usb/ohci \
	drv/bus/usb/uhci \
	drv/bus/usb/uhcirh \
	drv/bus/usb/usbflbk \
	drv/bus/usb/usbhid \
	drv/bus/usb/usbhub \
	drv/bus/usb/usbmast \
	drv/bus/usb/usbmid \
	drv/bus/usb/vhc \
	drv/nic/ne2k \
	drv/nic/e1k \
	drv/nic/rtl8139 \
	drv/nic/rtl8169 \
	drv/platform/icp

## Platform-specific hardware support
#

ifeq ($(UARCH), $(filter $(UARCH),amd64 ia32 ia64))
	DIRS += \
		drv/platform/pc \
		drv/bus/pci/pciintel \
		drv/bus/isa \
		drv/char/ns8250 \
		drv/time/cmos-rtc \
		srv/hw/irc/apic \
		srv/hw/irc/i8259
endif

ifeq ($(UARCH), $(filter $(UARCH),mips32 mips32eb))
	DIRS += \
		drv/platform/malta \
		drv/bus/pci/pciintel \
		drv/bus/isa
endif

ifeq ($(UARCH),ppc32)
	DIRS += \
		drv/platform/mac \
		drv/bus/pci/pciintel \
		srv/hw/bus/cuda_adb
endif

ifeq ($(UARCH),sparc64)
	DIRS += \
		srv/hw/irc/obio
endif

ifeq ($(UARCH),arm32)
	DIRS += \
		drv/platform/amdm37x \
		drv/fb/amdm37x_dispc \
		srv/hw/irc/icp-ic
endif

## System libraries
#

LIBC = lib/c
LIBS = \
	lib/fs \
	lib/block \
	lib/clui \
	lib/fmtutil \
	lib/scsi \
	lib/compress \
	lib/softint \
	lib/softfloat \
	lib/drv \
	lib/graph \
	lib/gui \
	lib/hound \
	lib/http \
	lib/softrend \
	lib/draw \
	lib/math \
	lib/net \
	lib/nic \
	lib/ext4 \
	lib/trackmod \
	lib/uri \
	lib/usb \
	lib/usbhost \
	lib/usbdev \
	lib/usbhid \
	lib/usbvirt \
	lib/pcm \
	lib/pcut \
	lib/bithenge \
	lib/posix \
	lib/mbr \
	lib/gpt

LIBC_BUILD = $(addsuffix .build,$(LIBC))
LIBS_BUILD = $(addsuffix .build,$(LIBS))
LIBN_BUILD = $(addsuffix .build,$(LIBN))
BUILDS := $(addsuffix .build,$(DIRS))
BUILDS_TESTS := $(addsuffix .build-test,$(DIRS) $(LIBS) lib/c)

CLEANS := $(addsuffix .clean,$(DIRS)) $(addsuffix .clean,$(LIBN)) $(addsuffix .clean,$(LIBS)) $(addsuffix .clean,$(LIBC))

.PHONY: all $(LIBC_BUILD) $(LIBS_BUILD) $(LIBN_BUILD) $(BUILDS) $(CLEANS) clean

all: $(BUILDS) $(BUILDS_TESTS)

$(BUILDS_TESTS): $(BUILDS)
	$(MAKE) -C $(basename $@) all-test PRECHECK=$(PRECHECK)

clean: $(CLEANS)

$(CLEANS):
	-$(MAKE) -C $(basename $@) clean

$(BUILDS): $(LIBC_BUILD) $(LIBS_BUILD) $(LIBN_BUILD)
	$(MAKE) -C $(basename $@) all PRECHECK=$(PRECHECK)

$(LIBN_BUILD): $(LIBC_BUILD) $(LIBS_BUILD)
	$(MAKE) -C $(basename $@) all PRECHECK=$(PRECHECK)

$(LIBS_BUILD): $(LIBC_BUILD)
	$(MAKE) -C $(basename $@) all PRECHECK=$(PRECHECK)

$(LIBC_BUILD):
	$(MAKE) -C $(basename $@) all PRECHECK=$(PRECHECK)
