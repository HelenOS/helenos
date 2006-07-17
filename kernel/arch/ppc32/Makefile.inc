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

BFD_NAME = elf32-powerpc
BFD_ARCH = powerpc:common
BFD = binary
TARGET = ppc-linux-gnu
TOOLCHAIN_DIR = /usr/local/ppc/bin

CFLAGS += -mcpu=powerpc -msoft-float -m32
AFLAGS += -a32
LFLAGS += -no-check-sections -N

DEFS += -D__32_BITS__

## Own configuration directives
#

CONFIG_FB = y

## Compile with hierarchical page tables support.
#

CONFIG_PAGE_PT = y
DEFS += -DCONFIG_PAGE_PT

## Compile with support for address space identifiers.
#

CONFIG_ASID = y
CONFIG_ASID_FIFO = y

## Compile with support for software integer division.
#

CONFIG_SOFTINT = y

ARCH_SOURCES = \
	arch/$(ARCH)/src/context.S \
	arch/$(ARCH)/src/debug/panic.s \
	arch/$(ARCH)/src/fpu_context.S \
	arch/$(ARCH)/src/boot/boot.S \
	arch/$(ARCH)/src/ppc32.c \
	arch/$(ARCH)/src/dummy.s \
	arch/$(ARCH)/src/exception.S \
	arch/$(ARCH)/src/interrupt.c \
	arch/$(ARCH)/src/asm.S \
	arch/$(ARCH)/src/cpu/cpu.c \
	arch/$(ARCH)/src/proc/scheduler.c \
	arch/$(ARCH)/src/ddi/ddi.c \
	arch/$(ARCH)/src/drivers/cuda.c \
	arch/$(ARCH)/src/mm/as.c \
	arch/$(ARCH)/src/mm/frame.c \
	arch/$(ARCH)/src/mm/memory_init.c \
	arch/$(ARCH)/src/mm/page.c \
	arch/$(ARCH)/src/mm/tlb.c \
	arch/$(ARCH)/src/drivers/pic.c
