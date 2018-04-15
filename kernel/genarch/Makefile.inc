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

## Accepted configuration directives
#

ifeq ($(CONFIG_ACPI),y)
GENARCH_SOURCES += \
	genarch/src/acpi/acpi.c \
	genarch/src/acpi/madt.c
endif

ifeq ($(CONFIG_PAGE_PT),y)
GENARCH_SOURCES += \
	genarch/src/mm/page_pt.c \
	genarch/src/mm/as_pt.c
endif

ifeq ($(CONFIG_PAGE_HT),y)
GENARCH_SOURCES += \
	genarch/src/mm/page_ht.c \
	genarch/src/mm/as_ht.c
endif

ifeq ($(CONFIG_ASID),y)
GENARCH_SOURCES += \
	genarch/src/mm/asid.c
endif

ifeq ($(CONFIG_ASID_FIFO),y)
GENARCH_SOURCES += \
	genarch/src/mm/asid_fifo.c
endif

ifeq ($(CONFIG_SOFTINT),y)
GENARCH_SOURCES += \
	genarch/src/softint/division.c \
	genarch/src/softint/multiplication.c
endif

ifeq ($(CONFIG_FB),y)
GENARCH_SOURCES += \
	genarch/src/fb/font-8x16.c \
	genarch/src/fb/fb.c \
	genarch/src/fb/bfb.c
endif

ifeq ($(CONFIG_DSRLNIN),y)
GENARCH_SOURCES += \
	genarch/src/drivers/dsrln/dsrlnin.c
endif

ifeq ($(CONFIG_DSRLNOUT),y)
GENARCH_SOURCES += \
	genarch/src/drivers/dsrln/dsrlnout.c
endif

ifeq ($(CONFIG_I8042),y)
GENARCH_SOURCES += \
	genarch/src/drivers/i8042/i8042.c
endif

ifeq ($(CONFIG_NS16550),y)
GENARCH_SOURCES += \
	genarch/src/drivers/ns16550/ns16550.c
endif

ifeq ($(CONFIG_PL011_UART),y)
GENARCH_SOURCES += \
	genarch/src/drivers/pl011/pl011.c
endif

ifeq ($(CONFIG_S3C24XX_IRQC),y)
GENARCH_SOURCES += \
	genarch/src/drivers/s3c24xx/irqc.c
endif

ifeq ($(CONFIG_S3C24XX_UART),y)
GENARCH_SOURCES += \
	genarch/src/drivers/s3c24xx/uart.c
endif

ifeq ($(CONFIG_OMAP_UART),y)
GENARCH_SOURCES += \
	genarch/src/drivers/omap/uart.c
endif

ifeq ($(CONFIG_AM335X_TIMERS),y)
GENARCH_SOURCES += \
	genarch/src/drivers/am335x/timer.c
endif

ifeq ($(CONFIG_BCM2835_MAILBOX),y)
GENARCH_SOURCES += \
	genarch/src/drivers/bcm2835/mbox.c
endif

ifeq ($(CONFIG_VIA_CUDA),y)
GENARCH_SOURCES += \
	genarch/src/drivers/via-cuda/cuda.c
endif

ifeq ($(CONFIG_PC_KBD),y)
GENARCH_SOURCES += \
	genarch/src/kbrd/kbrd.c \
	genarch/src/kbrd/scanc_pc.c
endif

ifeq ($(CONFIG_AT_KBD),y)
GENARCH_SOURCES += \
	genarch/src/kbrd/kbrd_at.c \
	genarch/src/kbrd/scanc_at.c
endif

ifeq ($(CONFIG_SUN_KBD),y)
GENARCH_SOURCES += \
	genarch/src/kbrd/kbrd.c \
	genarch/src/kbrd/scanc_sun.c
endif

ifeq ($(CONFIG_MAC_KBD),y)
GENARCH_SOURCES += \
	genarch/src/kbrd/kbrd.c \
	genarch/src/kbrd/scanc_mac.c
endif

ifeq ($(CONFIG_SRLN),y)
GENARCH_SOURCES += \
	genarch/src/srln/srln.c
endif

ifeq ($(CONFIG_OFW_TREE),y)
GENARCH_SOURCES += \
	genarch/src/ofw/ofw_tree.c
endif

ifeq ($(CONFIG_OFW_PCI),y)
GENARCH_SOURCES += \
	genarch/src/ofw/ebus.c \
	genarch/src/ofw/pci.c  \
	genarch/src/ofw/sbus.c \
	genarch/src/ofw/upa.c
endif

ifeq ($(CONFIG_MULTIBOOT), y)
GENARCH_SOURCES += \
	genarch/src/multiboot/common.c \
	genarch/src/multiboot/multiboot.c \
	genarch/src/multiboot/multiboot2.c

GENARCH_AUTOCHECK_HEADERS += \
	genarch/include/genarch/multiboot/multiboot_memmap_struct.h \
	genarch/include/genarch/multiboot/multiboot_info_struct.h
endif

ifeq ($(CONFIG_EGA), y)
GENARCH_SOURCES += \
	genarch/src/drivers/ega/ega.c
endif

ifeq ($(CONFIG_IOMAP_BITMAP), y)
GENARCH_SOURCES += \
	genarch/src/ddi/ddi-bitmap.c
endif

ifeq ($(CONFIG_IOMAP_DUMMY), y)
GENARCH_SOURCES += \
	genarch/src/ddi/ddi-dummy.c
endif

