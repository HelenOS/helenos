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

BFD_ARCH = mips
TARGET = mipsel-linux-gnu
TOOLCHAIN_DIR = /usr/local/mipsel

KERNEL_LOAD_ADDRESS = 0x80100000
INIT_ADDRESS = 0x81000000
INIT_SIZE = 262144

GCC_CFLAGS += -mno-abicalls -G 0 -fno-zero-initialized-in-bss

DEFS += -D__32_BITS__ -DMACHINE=$(MACHINE) -DKERNEL_LOAD_ADDRESS=${KERNEL_LOAD_ADDRESS} -DINIT_ADDRESS=${INIT_ADDRESS} -DINIT_SIZE=${INIT_SIZE}

## Compile with hierarchical page tables support.
#

CONFIG_PAGE_PT = y
DEFS += -DCONFIG_PAGE_PT

## Compile with support for address space identifiers.
#

CONFIG_ASID = y
CONFIG_ASID_FIFO = y

## Accepted MACHINEs
#

ifeq ($(MACHINE),indy)
	# GCC 4.0.1 compiled for mipsEL has problems compiling in 
	# BigEndian mode with the swl/swr/lwl/lwr instructions.
	# We have to compile it with mips-sgi-irix5 to get it right.
	
	BFD_NAME = elf32-bigmips
	BFD = ecoff-bigmips --impure
	TARGET = mips-sgi-irix5
	TOOLCHAIN_DIR = /usr/local/mips/bin
	KERNEL_LOAD_ADDRESS = 0x88002000
	GCC_CFLAGS += -EB -DBIG_ENDIAN -DARCH_HAS_FPU -march=r4600
	INIT_ADDRESS = 0
	INIT_SIZE = 0
endif
ifeq ($(MACHINE),lgxemul)
	BFD_NAME = elf32-tradlittlemips
	BFD = binary
	CFLAGS += -DFB_INVERT_ENDIAN -DARCH_HAS_FPU -mips3
endif
ifeq ($(MACHINE),bgxemul)
	BFD_NAME = elf32-bigmips
	BFD = ecoff-bigmips
	TARGET = mips-sgi-irix5
	TOOLCHAIN_DIR = /usr/local/mips/bin
	GCC_CFLAGS += -EB -DBIG_ENDIAN -DARCH_HAS_FPU -mips3
	INIT_ADDRESS = 0x81800000
endif
ifeq ($(MACHINE),simics)
	# SIMICS 4kc emulation is broken, although for instructions
	# that do not bother us
	
	BFD_NAME = elf32-tradlittlemips
	BFD = elf32-tradlittlemips
	GCC_CFLAGS += -mhard-float -mips3 -DTLBCNT=16
	TLBCNT = 16
endif
ifeq ($(MACHINE),msim)
	BFD_NAME = elf32-tradlittlemips
	BFD = binary
	GCC_CFLAGS += -mhard-float -mips3
endif

## Compile with support for software integer division.
#

CONFIG_SOFTINT = y


ARCH_SOURCES = \
	arch/$(ARCH)/src/start.S \
	arch/$(ARCH)/src/context.S \
	arch/$(ARCH)/src/panic.S \
	arch/$(ARCH)/src/mips32.c \
	arch/$(ARCH)/src/dummy.S \
	arch/$(ARCH)/src/console.c \
	arch/$(ARCH)/src/asm.S \
	arch/$(ARCH)/src/exception.c \
	arch/$(ARCH)/src/interrupt.c \
	arch/$(ARCH)/src/cache.c \
	arch/$(ARCH)/src/debugger.c \
	arch/$(ARCH)/src/cpu/cpu.c \
	arch/$(ARCH)/src/mm/frame.c \
	arch/$(ARCH)/src/mm/page.c \
	arch/$(ARCH)/src/mm/tlb.c \
	arch/$(ARCH)/src/mm/as.c \
	arch/$(ARCH)/src/fpu_context.c \
	arch/$(ARCH)/src/ddi/ddi.c \
	arch/$(ARCH)/src/drivers/arc.c \
	arch/$(ARCH)/src/drivers/msim.c \
	arch/$(ARCH)/src/drivers/serial.c
