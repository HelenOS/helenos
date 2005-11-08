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

BFD_NAME = elf32-i386
BFD_ARCH = mips
TARGET = mipsel-linux-gnu
TOOLCHAIN_DIR = /usr/local/mipsel/bin

## Make some default assumptions
#

ifndef CPU
	CPU = pentium4
endif

## Accepted CPUs
#

ifeq ($(CPU),athlon-xp)
	CFLAGS += -march=athlon-xp -mmmx -msse -m3dnow
	DEFS += -DFENCES=486
	CONFIG_SMP = n
	CONFIG_HT = n
endif
ifeq ($(CPU),athlon-mp)
	CFLAGS += -march=athlon-mp -mmmx -msse -m3dnow
	DEFS += -DFENCES=486
endif
ifeq ($(CPU),pentium3)
	CFLAGS += -march=pentium3 -mmmx -msse -msse2
	DEFS += -DFENCES=486
endif
ifeq ($(CPU),pentium4)
	CFLAGS += -march=pentium4 -mfpmath=sse -mmmx -msse -msse2 -msse3
	DEFS += -DFENCES=p4
endif

## Own configuration directives
#

CONFIG_ACPI = y

## Accepted configuration directives
#

ifeq ($(CONFIG_SMP),y)
	DEFS += -DSMP
endif
ifeq ($(CONFIG_HT),y)
	DEFS += -DHT
endif
ifeq ($(CONFIG_FPU_LAZY),y)
	DEFS += -DFPU_LAZY
endif

ARCH_SOURCES = \
	arch/$(ARCH)/src/context.s \
	arch/$(ARCH)/src/debug/panic.s \
	arch/$(ARCH)/src/delay.s \
	arch/$(ARCH)/src/asm.S \
	arch/$(ARCH)/src/proc/scheduler.c \
	arch/$(ARCH)/src/bios/bios.c \
	arch/$(ARCH)/src/smp/ap.S \
	arch/$(ARCH)/src/smp/apic.c \
	arch/$(ARCH)/src/smp/mps.c \
	arch/$(ARCH)/src/smp/smp.c \
	arch/$(ARCH)/src/atomic.S \
	arch/$(ARCH)/src/smp/ipi.c \
	arch/$(ARCH)/src/ia32.c \
	arch/$(ARCH)/src/interrupt.c \
	arch/$(ARCH)/src/pm.c \
	arch/$(ARCH)/src/userspace.c \
	arch/$(ARCH)/src/cpu/cpu.c \
	arch/$(ARCH)/src/mm/frame.c \
	arch/$(ARCH)/src/mm/memory_init.c \
	arch/$(ARCH)/src/mm/page.c \
	arch/$(ARCH)/src/mm/tlb.c \
	arch/$(ARCH)/src/drivers/i8042.c \
	arch/$(ARCH)/src/drivers/i8254.c \
	arch/$(ARCH)/src/drivers/i8259.c \
	arch/$(ARCH)/src/drivers/ega.c \
	arch/$(ARCH)/src/boot/boot.S \
	arch/$(ARCH)/src/boot/memmap.S \
	arch/$(ARCH)/src/fpu_context.c\
	arch/$(ARCH)/src/fmath.c
















DEFS= -DMACHINE=${MACHINE} -DKERNEL_LOAD_ADDRESS=${KERNEL_LOAD_ADDRESS}
CFLAGS=-mno-abicalls -G 0 -nostdlib -fno-builtin -O2  -fno-zero-initialized-in-bss
LFLAGS=

# GCC 4.0.1 compiled for mipsEL has problems compiling in 
# BigEndian mode with the swl/swr/lwl/lwr instructions.
# We have to compile it with mips-sgi-irix5 to get it right.
ifeq (${MACHINE},indy)
 MIPS_TARGET=mips-sgi-irix5
 MIPS_CC_DIR=/usr/local/mips/bin
 MIPS_BINUTILS_DIR=/usr/local/mips/bin

 CFLAGS += -EB -DBIG_ENDIAN -DHAVE_FPU -DFPU_LAZY -march=r4600
 BFD = ecoff-bigmips
 KERNEL_LOAD_ADDRESS = 0x88002000
 BFD_NAME=elf32-bigmips
endif

ifeq (${MACHINE},lgxemul)
 CFLAGS += -DHAVE_FPU -DFPU_LAZY -mips3
 BFD = ecoff-littlemips
 KERNEL_LOAD_ADDRESS = 0x80100000
 BFD_NAME=elf32-tradlittlemips
endif

ifeq (${MACHINE},bgxemul)
 MIPS_TARGET=mips-sgi-irix5
 MIPS_CC_DIR=/usr/local/mips/bin
 MIPS_BINUTILS_DIR=/usr/local/mips/bin

 CFLAGS += -EB -DBIG_ENDIAN -DHAVE_FPU -DFPU_LAZY -mips3
 BFD = ecoff-bigmips
 KERNEL_LOAD_ADDRESS = 0x80100000
 BFD_NAME=elf32-bigmips
endif

# MSIM needs lwl/swl patch & 4kc instruction patch to work
# otherwise add -mmemcpy -mips3
ifeq (${MACHINE},msim4kc)
 BFD = binary
 CFLAGS += -mhard-float -march=4kc 
 KERNEL_LOAD_ADDRESS = 0x80100000
 BFD_NAME=elf32-tradlittlemips
endif

ifeq (${MACHINE},msim)
 BFD = binary
 CFLAGS += -mhard-float -mips3
 KERNEL_LOAD_ADDRESS = 0x80100000
 BFD_NAME=elf32-tradlittlemips
endif

# SIMICS 4kc emulation is broken, although for instructions
# that do not bother us
ifeq (${MACHINE},simics)
 BFD = elf32-tradlittlemips
 CFLAGS += -mhard-float -mips3
 KERNEL_LOAD_ADDRESS = 0x80100000
 BFD_NAME=elf32-tradlittlemips
endif

arch/$(ARCH)/_link.ld: arch/$(ARCH)/_link.ld.in
	$(CC) $(CFLAGS) -C -DBFD=${BFD} -E -x c $< | grep -v "^\#" > $@

arch_sources= \
	generic/src/arch/start.S \
	generic/src/arch/context.S \
	generic/src/arch/panic.S \
	generic/src/arch/mips32.c \
	generic/src/arch/dummy.S \
	generic/src/arch/console.c \
	generic/src/arch/asm.S \
	generic/src/arch/exception.c \
	generic/src/arch/interrupt.c \
	generic/src/arch/cache.c \
	generic/src/arch/cpu/cpu.c \
	generic/src/arch/mm/asid.c \
	generic/src/arch/mm/frame.c \
	generic/src/arch/mm/page.c \
	generic/src/arch/mm/tlb.c \
	generic/src/arch/mm/vm.c \
	generic/src/arch/fpu_context.c \
	generic/src/arch/fmath.c \
	generic/src/arch/drivers/arc.c
