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

## Toolchain configuration
#

ifndef CROSS_PREFIX
	CROSS_PREFIX = /usr/local
endif

BFD_NAME = elf64-little
BFD_ARCH = ia64-elf64
TARGET = ia64-pc-linux-gnu
TOOLCHAIN_DIR = $(CROSS_PREFIX)/ia64

CMN1 = -mconstant-gp -fno-unwind-tables -mfixed-range=f32-f127
GCC_CFLAGS += $(CMN1)
ICC_CFLAGS += $(CMN1)

LFLAGS += -EL
AFLAGS += -mconstant-gp

DEFS += -D__64_BITS__ -D$(MACHINE)

## Compile with page hash table support.
#

CONFIG_PAGE_HT = y
DEFS += -DCONFIG_PAGE_HT

## Compile with support for address space identifiers.
#

CONFIG_ASID = y
CONFIG_ASID_FIFO = y


## Compile with support for software integer division.
#

CONFIG_SOFTINT = y

ARCH_SOURCES = \
	arch/$(ARCH)/src/start.S \
	arch/$(ARCH)/src/asm.S \
	arch/$(ARCH)/src/dummy.s \
	arch/$(ARCH)/src/ia64.c \
	arch/$(ARCH)/src/fpu_context.c \
	arch/$(ARCH)/src/context.S \
	arch/$(ARCH)/src/cpu/cpu.c \
	arch/$(ARCH)/src/ivt.S \
	arch/$(ARCH)/src/interrupt.c \
	arch/$(ARCH)/src/mm/as.c \
	arch/$(ARCH)/src/mm/frame.c \
	arch/$(ARCH)/src/mm/page.c \
	arch/$(ARCH)/src/mm/tlb.c \
	arch/$(ARCH)/src/mm/vhpt.c \
	arch/$(ARCH)/src/proc/scheduler.c \
	arch/$(ARCH)/src/ddi/ddi.c \
	arch/$(ARCH)/src/smp/smp.c \
	arch/$(ARCH)/src/drivers/it.c

ifeq ($(MACHINE),ski)
	ARCH_SOURCES += arch/$(ARCH)/src/ski/ski.c 
	DEFS += -DSKI
	BFD = binary
endif

ifeq ($(MACHINE),i460GX)
	ARCH_SOURCES += arch/$(ARCH)/src/drivers/ega.c
	CONFIG_I8042 = y
	DEFS += -DI460GX -DCONFIG_I8042
	BFD = binary
endif

ifeq ($(CONFIG_SMP),y)
	DEFS += -DCONFIG_SMP
endif

