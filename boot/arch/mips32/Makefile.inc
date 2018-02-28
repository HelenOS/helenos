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

BFD_ARCH = mips
BITS = 32
EXTRA_CFLAGS = -msoft-float -mno-abicalls -G 0 -fno-zero-initialized-in-bss -mabi=32
AS_PROLOG = .module softfloat;

ifeq ($(MACHINE),msim)
	BFD_NAME = elf32-tradlittlemips
	BFD_OUTPUT = binary
	ENDIANESS = LE
	EXTRA_CFLAGS += -march=r4000
endif
ifeq ($(MACHINE),lmalta)
	BFD_NAME = elf32-tradlittlemips
	BFD_OUTPUT = elf32-tradlittlemips
	ENDIANESS = LE
	EXTRA_CFLAGS += -march=4kc
endif
ifeq ($(MACHINE),bmalta)
	BFD_NAME = elf32-tradbigmips
	BFD_OUTPUT = elf32-tradbigmips
	ENDIANESS = BE
	EXTRA_CFLAGS += -march=4kc
endif

ifeq ($(MACHINE), $(filter $(MACHINE),bmalta lmalta))
	RD_DRVS_ESSENTIAL += \
		platform/malta \
		block/ata_bd \
		bus/pci/pciintel \
		bus/isa \
		char/i8042 \
		hid/ps2mouse \
		hid/xtkbd

	RD_DRV_CFG += \
		bus/isa
endif

ifeq ($(MACHINE),msim)
	RD_DRVS_ESSENTIAL += \
		platform/msim \
		block/ddisk \
		char/msim-con
endif

SOURCES = \
	arch/$(BARCH)/src/asm.S \
	arch/$(BARCH)/src/main.c \
	arch/$(BARCH)/src/putchar.c \
	$(COMPS).s \
	$(COMPS)_desc.c \
	genarch/src/division.c \
	genarch/src/multiplication.c \
	generic/src/memstr.c \
	generic/src/printf_core.c \
	generic/src/vprintf.c \
	generic/src/printf.c \
	generic/src/str.c \
	generic/src/version.c \
	generic/src/inflate.c

PRE_DEPEND = $(COMPS).s $(COMPS).h $(COMPS)_desc.c $(COMPONENTS_DEFLATE)
