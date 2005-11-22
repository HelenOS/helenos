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

BFD_ARCH = mips
TARGET = mipsel-linux-gnu
TOOLCHAIN_DIR = /usr/local/mipsel/bin

## Make some default assumptions
#

ifndef MACHINE
	MACHINE = msim
endif

KERNEL_LOAD_ADDRESS = 0x80100000
INIT_ADDRESS = 0x80110000
INIT_SIZE = 65536
CFLAGS += -mno-abicalls -G 0 -fno-zero-initialized-in-bss
DEFS += -DMACHINE=${MACHINE} -DKERNEL_LOAD_ADDRESS=${KERNEL_LOAD_ADDRESS} -DINIT_ADDRESS=${INIT_ADDRESS} -DINIT_SIZE=${INIT_SIZE}

## Accepted MACHINEs
#

ifeq ($(MACHINE),indy)
	# GCC 4.0.1 compiled for mipsEL has problems compiling in 
	# BigEndian mode with the swl/swr/lwl/lwr instructions.
	# We have to compile it with mips-sgi-irix5 to get it right.
	
	BFD_NAME = elf32-bigmips
	BFD = ecoff-bigmips
	TARGET = mips-sgi-irix5
	TOOLCHAIN_DIR = /usr/local/mips/bin
	KERNEL_LOAD_ADDRESS = 0x88002000
	CFLAGS += -EB -DBIG_ENDIAN -DHAVE_FPU -march=r4600
endif
ifeq ($(MACHINE}),lgxemul)
	BFD_NAME=elf32-tradlittlemips
	BFD = ecoff-littlemips
	CFLAGS += -DHAVE_FPU -mips3
endif
ifeq ($(MACHINE),bgxemul)
	BFD_NAME=elf32-bigmips
	BFD = ecoff-bigmips
	TARGET = mips-sgi-irix5
	TOOLCHAIN_DIR = /usr/local/mips/bin
	CFLAGS += -EB -DBIG_ENDIAN -DHAVE_FPU -mips3
endif
ifeq ($(MACHINE),msim4kc)
	# MSIM needs lwl/swl patch & 4kc instruction patch to work
	# otherwise add -mmemcpy -mips3
	
	BFD_NAME = elf32-tradlittlemips
	BFD = binary
	CFLAGS += -mhard-float -march=4kc 
endif
ifeq ($(MACHINE),simics)
	# SIMICS 4kc emulation is broken, although for instructions
	# that do not bother us
	
	BFD_NAME = elf32-tradlittlemips
	BFD = elf32-tradlittlemips
	CFLAGS += -mhard-float -mips3
endif
ifeq ($(MACHINE),msim)
	BFD_NAME = elf32-tradlittlemips
	BFD = binary
	CFLAGS += -mhard-float -mips3
endif

## Own configuration directives
#

CONFIG_OFW = y

## Accepted configuration directives
#

ifeq ($(CONFIG_FPU_LAZY),y)
	DEFS += -DCONFIG_FPU_LAZY
endif

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
	arch/$(ARCH)/src/cpu/cpu.c \
	arch/$(ARCH)/src/mm/asid.c \
	arch/$(ARCH)/src/mm/frame.c \
	arch/$(ARCH)/src/mm/page.c \
	arch/$(ARCH)/src/mm/tlb.c \
	arch/$(ARCH)/src/mm/vm.c \
	arch/$(ARCH)/src/fpu_context.c \
	arch/$(ARCH)/src/fmath.c \
	arch/$(ARCH)/src/drivers/arc.c \
	arch/$(ARCH)/src/drivers/keyboard.c
