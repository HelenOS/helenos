#
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

## Toolchain configuration
#

BFD_NAME = elf64-little
BFD_ARCH = ia64-elf64
BFD = elf64-ia64-little
TARGET = ia64-pc-linux-gnu
TOOLCHAIN_DIR = /usr/local/ia64/bin

## Make some default assumptions
#

INIT0_ADDRESS = 0xe000000000400000
INIT0_SIZE = 0x100000

INIT1_ADDRESS = 0xe000000000800000
INIT1_SIZE = 0x100000

CFLAGS += -mconstant-gp -fno-unwind-tables -mfixed-range=f32-f127
LFLAGS += -EL
AFLAGS += -mconstant-gp

DEFS += -D__64_BITS__ -DINIT0_ADDRESS=$(INIT0_ADDRESS) -DINIT0_SIZE=$(INIT0_SIZE) \
	-DINIT1_ADDRESS=$(INIT1_ADDRESS) -DINIT1_SIZE=$(INIT1_SIZE)

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
	arch/$(ARCH)/src/ski/ski.c \
	arch/$(ARCH)/src/cpu/cpu.c \
	arch/$(ARCH)/src/ivt.S \
	arch/$(ARCH)/src/interrupt.c \
	arch/$(ARCH)/src/mm/as.c \
	arch/$(ARCH)/src/mm/frame.c \
	arch/$(ARCH)/src/mm/page.c \
	arch/$(ARCH)/src/mm/tlb.c \
	arch/$(ARCH)/src/proc/scheduler.c \
	arch/$(ARCH)/src/drivers/it.c
