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
BFD_ARCH = i386
TARGET = i686-pc-linux-gnu
TOOLCHAIN_DIR = /usr/local/i686/bin

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
