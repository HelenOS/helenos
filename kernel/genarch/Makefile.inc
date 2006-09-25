# Copyright (C) 2005 Martin Decky
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
		genarch/src/softint/division.c
endif

## Framebuffer
ifeq ($(CONFIG_FB),y)
	GENARCH_SOURCES += \
		genarch/src/fb/font-8x16.c \
		genarch/src/fb/fb.c
	DEFS += -DCONFIG_FB
endif

## i8042 controller
ifeq ($(CONFIG_I8042),y)
	GENARCH_SOURCES += \
		genarch/src/kbd/i8042.c \
		genarch/src/kbd/key.c \
		genarch/src/kbd/scanc_pc.c
endif

## Sun keyboard
ifeq ($(CONFIG_SUN_KBD),y)
	GENARCH_SOURCES += \
		genarch/src/kbd/key.c \
		genarch/src/kbd/scanc_sun.c
endif

## z8530 controller
ifeq ($(CONFIG_Z8530),y)
	GENARCH_SOURCES += \
		genarch/src/kbd/z8530.c
endif

## ns16550 controller
ifeq ($(CONFIG_NS16550),y)
	GENARCH_SOURCES += \
		genarch/src/kbd/ns16550.c
endif


## OpenFirmware Device Tree
ifeq ($(CONFIG_OFW_TREE), y)
	GENARCH_SOURCES += \
		genarch/src/ofw/ofw_tree.c \
		genarch/src/ofw/ebus.c \
		genarch/src/ofw/fhc.c \
		genarch/src/ofw/pci.c  \
		genarch/src/ofw/upa.c 
endif
