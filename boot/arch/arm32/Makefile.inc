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

ifeq ($(MACHINE), gta02)
	BOOT_OUTPUT = image.boot
	POST_OUTPUT = $(ROOT_PATH)/uImage.bin
	LADDR = 0x30008000
	SADDR = 0x30008000
	POSTBUILD = Makefile.uboot
endif

ifeq ($(MACHINE), $(filter $(MACHINE),beagleboardxm beaglebone))
	BOOT_OUTPUT = image.boot
	POST_OUTPUT = $(ROOT_PATH)/uImage.bin
	LADDR = 0x80000000
	SADDR = 0x80000000
	POSTBUILD = Makefile.uboot
endif

ifeq ($(MACHINE), raspberrypi)
	BOOT_OUTPUT = image.boot
	POST_OUTPUT = $(ROOT_PATH)/uImage.bin
	LADDR = 0x00008000
	SADDR = 0x00008000
	POSTBUILD = Makefile.uboot
endif

BFD_NAME = elf32-littlearm
BFD_OUTPUT = $(BFD_NAME)
BFD_ARCH = arm

BITS = 32
ENDIANESS = LE
EXTRA_CFLAGS = -march=$(subst _,-,$(PROCESSOR_ARCH)) -mno-unaligned-access

ifeq ($(MACHINE), gta02)
	RD_SRVS_ESSENTIAL += \
		$(USPACE_PATH)/srv/hid/s3c24xx_ts/s3c24xx_ts \
		$(USPACE_PATH)/srv/hw/char/s3c24xx_uart/s3c24ser
endif

ifeq ($(MACHINE), $(filter $(MACHINE),beagleboardxm beaglebone))
	RD_DRVS_ESSENTIAL += \
		platform/amdm37x \
		fb/amdm37x_dispc
endif

ifeq ($(MACHINE), integratorcp)
	RD_DRVS_ESSENTIAL += \
		char/pl050 \
		hid/atkbd \
		hid/ps2mouse \
		intctl/icp-ic \
		platform/icp
endif

RD_DRVS_ESSENTIAL += \
	bus/usb/ehci \
	bus/usb/ohci \
	bus/usb/usbdiag \
	bus/usb/usbflbk \
	bus/usb/usbhub \
	bus/usb/usbmid \
	block/usbmast \
	hid/usbhid

SOURCES = \
	arch/$(BARCH)/src/asm.S \
	arch/$(BARCH)/src/eabi.S \
	arch/$(BARCH)/src/main.c \
	arch/$(BARCH)/src/mm.c \
	arch/$(BARCH)/src/putchar.c \
	$(COMPS).s \
	$(COMPS)_desc.c \
	genarch/src/division.c \
	generic/src/memstr.c \
	generic/src/printf_core.c \
	generic/src/vprintf.c \
	generic/src/printf.c \
	generic/src/str.c \
	generic/src/version.c \
	generic/src/inflate.c

PRE_DEPEND = $(COMPS).s $(COMPS).h $(COMPS)_desc.c $(COMPONENTS_DEFLATE)
