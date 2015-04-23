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

RD_SRVS_ESSENTIAL += \
	$(USPACE_PATH)/srv/audio/hound/hound \
	$(USPACE_PATH)/srv/devman/devman \
	$(USPACE_PATH)/srv/hw/irc/apic/apic \
	$(USPACE_PATH)/srv/hw/irc/i8259/i8259


RD_DRVS_ESSENTIAL += \
	platform/pc \
	block/ata_bd \
	bus/pci/pciintel \
	bus/isa \
	audio/sb16 \
	char/i8042 \
	char/ps2mouse \
	char/xtkbd

RD_DRVS_NON_ESSENTIAL += \
	audio/hdaudio \
	char/ns8250 \
	time/cmos-rtc \
	bus/usb/ehci\
	bus/usb/ohci \
	bus/usb/uhci \
	bus/usb/uhcirh \
	bus/usb/usbflbk \
	bus/usb/usbhub \
	bus/usb/usbhid \
	bus/usb/usbmast \
	bus/usb/usbmid \
	bus/usb/vhc

RD_DRV_CFG += \
	bus/isa

RD_APPS_ESSENTIAL += \
	$(USPACE_PATH)/app/edit/edit \
	$(USPACE_PATH)/app/mixerctl/mixerctl \
	$(USPACE_PATH)/app/wavplay/wavplay

BOOT_OUTPUT = $(ROOT_PATH)/image.iso
PREBUILD = $(INITRD).img
BUILD = Makefile.grub
