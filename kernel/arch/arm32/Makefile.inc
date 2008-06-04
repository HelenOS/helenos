#
# Copyright (c) 2007 Jakub Jermar, Michal Kebrt
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

BFD_NAME = elf32-littlearm
BFD_ARCH = arm
BFD = binary
TARGET = arm-linux-gnu
TOOLCHAIN_DIR = /usr/local/arm

KERNEL_LOAD_ADDRESS = 0x80200000

# ifeq ($(MACHINE), gxemul_testarm)
	DMACHINE = MACHINE_GXEMUL_TESTARM
# endif

ATSIGN = %

GCC_CFLAGS += -fno-zero-initialized-in-bss

DEFS += -D__32_BITS__ -DKERNEL_LOAD_ADDRESS=$(KERNEL_LOAD_ADDRESS) -D$(DMACHINE)

# Compile with framebuffer support

ifeq ($(CONFIG_FB), y)
	DEFS += -DCONFIG_FB -DFB_INVERT_ENDIAN
endif

## Compile with hierarchical page tables support.
#

CONFIG_PAGE_PT = y
DEFS += -DCONFIG_PAGE_PT

## Compile with support for address space identifiers.
#
# no HW support for ASIDs
#CONFIG_ASID = y
#CONFIG_ASID_FIFO = y 

## Compile with support with software division and multiplication.
#

CONFIG_SOFTINT = y

ARCH_SOURCES = \
	arch/$(ARCH)/src/start.S \
	arch/$(ARCH)/src/asm.S \
	arch/$(ARCH)/src/arm32.c \
	arch/$(ARCH)/src/context.S \
	arch/$(ARCH)/src/dummy.S \
	arch/$(ARCH)/src/panic.S \
	arch/$(ARCH)/src/cpu/cpu.c \
	arch/$(ARCH)/src/ddi/ddi.c \
	arch/$(ARCH)/src/interrupt.c \
	arch/$(ARCH)/src/debug/print.c \
	arch/$(ARCH)/src/console.c \
	arch/$(ARCH)/src/exception.c \
	arch/$(ARCH)/src/userspace.c \
	arch/$(ARCH)/src/mm/as.c \
	arch/$(ARCH)/src/mm/frame.c \
	arch/$(ARCH)/src/mm/page.c \
	arch/$(ARCH)/src/mm/tlb.c \
	arch/$(ARCH)/src/mm/page_fault.c

# ifeq ($(MACHINE), gxemul_testarm)
	ARCH_SOURCES += arch/$(ARCH)/src/drivers/gxemul.c
# endif

