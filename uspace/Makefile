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
	app/cpptest \
	app/devctl \
	app/dnscfg \
	app/dnsres \
	app/download \
	app/edit \
	app/fdisk \
	app/fontviewer \
	app/getterm \
	app/gunzip \
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
	app/netecho \
	app/nterm \
	app/redir \
	app/rcutest \
	app/rcubench \
	app/sbi \
	app/sportdmp \
	app/stats \
	app/taskdump \
	app/tester \
	app/testread \
	app/testwrit \
	app/tetris \
	app/tmon \
	app/trace \
	app/top \
	app/untar \
	app/usbinfo \
	app/vol \
	app/vuhid \
	app/nic \
	app/ping \
	app/pkg \
	app/sysinfo \
	app/sysinst \
	app/mkbd \
	app/date \
	app/vcalc \
	app/vdemo \
	app/viewer \
	app/vlaunch \
	app/vterm \
	app/df \
	app/wavplay \
	app/websrv \
	app/wifi_supplicant \
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
	srv/bd/vbd \
	srv/fs/exfat \
	srv/fs/udf \
	srv/fs/fat \
	srv/fs/cdfs \
	srv/fs/tmpfs \
	srv/fs/mfs \
	srv/fs/locfs \
	srv/fs/ext4fs \
	srv/volsrv \
	srv/hid/compositor \
	srv/hid/console \
	srv/hid/s3c24xx_ts \
	srv/hid/isdv4_tablet \
	srv/hid/input \
	srv/hid/output \
	srv/hid/remcons \
	srv/hw/char/s3c24xx_uart \
	srv/hid/rfb \
	srv/test/chardev-test \
	drv/audio/hdaudio \
	drv/audio/sb16 \
	drv/root/root \
	drv/root/virt \
	drv/block/ahci \
	drv/block/ata_bd \
	drv/block/ddisk \
	drv/block/usbmast \
	drv/bus/adb/cuda_adb \
	drv/bus/isa \
	drv/bus/pci/pciintel \
	drv/bus/usb/ehci \
	drv/bus/usb/ohci \
	drv/bus/usb/uhci \
	drv/bus/usb/usbdiag \
	drv/bus/usb/usbflbk \
	drv/bus/usb/usbhub \
	drv/bus/usb/usbmid \
	drv/bus/usb/vhc \
	drv/bus/usb/xhci \
	drv/char/i8042 \
	drv/char/msim-con \
	drv/char/ns8250 \
	drv/char/pl050 \
	drv/char/ski-con \
	drv/char/sun4v-con \
	drv/fb/amdm37x_dispc \
	drv/fb/kfb \
	drv/hid/adb-kbd \
	drv/hid/adb-mouse \
	drv/hid/atkbd \
	drv/hid/ps2mouse \
	drv/hid/usbhid \
	drv/hid/xtkbd \
	drv/intctl/apic \
	drv/intctl/i8259 \
	drv/intctl/icp-ic \
	drv/intctl/obio \
	drv/nic/ne2k \
	drv/nic/e1k \
	drv/nic/rtl8139 \
	drv/nic/rtl8169 \
	drv/nic/ar9271 \
	drv/nic/virtio-net \
	drv/platform/amdm37x \
	drv/platform/icp \
	drv/platform/mac \
	drv/platform/malta \
	drv/platform/msim \
	drv/platform/pc \
	drv/platform/ski \
	drv/platform/sun4v \
	drv/test/test1 \
	drv/test/test2 \
	drv/test/test3 \
	drv/time/cmos-rtc

## sun4u driver won't compile on 32-bit targets
ifeq ($(UARCH),sparc64)
	DIRS += \
		drv/platform/sun4u
endif

## Dynamic linking tests
#
ifeq ($(CONFIG_BUILD_SHARED_LIBS),y)
	DIRS += \
		app/dltest \
		app/dltests
endif

## System libraries
#

BASE_LIBS = \
	lib/c \
	lib/cpp \
	lib/softint \
	lib/softfloat \
	lib/untar

LIBS = \
	lib/fs \
	lib/block \
	lib/crypto \
	lib/clui \
	lib/dltest \
	lib/fdisk \
	lib/fmtutil \
	lib/scsi \
	lib/compress \
	lib/drv \
	lib/graph \
	lib/gui \
	lib/hound \
	lib/http \
	lib/softrend \
	lib/draw \
	lib/label \
	lib/math \
	lib/minix \
	lib/nettl \
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
	lib/ieee80211 \
	lib/virtio

BASE_BUILDS := $(addsuffix .build,$(BASE_LIBS))
BUILDS := $(addsuffix .build,$(DIRS) $(LIBS))
BUILDS_TESTS := $(addsuffix .build-test,$(DIRS) $(LIBS) $(BASE_LIBS))
DEPS = $(addsuffix /deps.mk,$(DIRS) $(LIBS))
CLEANS := $(addsuffix .clean,$(DIRS) $(LIBS) $(BASE_LIBS))

.PHONY: all $(BASE_BUILDS) $(BUILDS) $(BUILDS_TESTS) $(CLEANS) clean export

all: $(BUILDS) $(BUILDS_TESTS)

$(BUILDS_TESTS): $(BASE_BUILDS) $(BUILDS)
	$(MAKE) -r -C $(basename $@) all-test PRECHECK=$(PRECHECK)

export: lib/posix.build lib/math.build lib/clui.build
	$(MAKE) -r -C lib/posix export EXPORT_DIR=$(EXPORT_DIR)

clean: $(CLEANS)
	find lib app drv srv -name '*.o' -follow -exec rm \{\} \;
	find lib app drv srv -name '*.lo' -follow -exec rm \{\} \;
	find lib app drv srv -name '*.d' -follow -exec rm \{\} \;

$(CLEANS):
	-$(MAKE) -r -C $(basename $@) fasterclean

$(BASE_BUILDS) $(BUILDS):
	$(MAKE) -r -C $(basename $@) all PRECHECK=$(PRECHECK)

$(DEPS): %/deps.mk: %/Makefile
	$(MAKE) -r -C $(@D) deps.mk SELF_TARGET="$(@D).build"

# Special case for base libraries.
lib/c.build: lib/softfloat.build lib/softint.build

$(BUILDS): $(BASE_BUILDS)

-include $(DEPS)
