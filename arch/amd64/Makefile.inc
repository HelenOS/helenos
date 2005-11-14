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

BFD_NAME = elf64-x86-64
BFD_ARCH = i386:x86-64
BFD = binary
TARGET = amd64-linux-gnu
TOOLCHAIN_DIR = /usr/local/amd64/bin

## Make some default assumptions
#

ifndef CPU
	CPU = opteron
endif

CFLAGS += -fno-unwind-tables -m64 -mcmodel=kernel -mno-red-zone -O3
DEFS += -D_CPU=${CPU}

## Accepted CPUs
#

ifeq ($(CPU),opteron)
	CFLAGS += -march=opteron
	DEFS += -DFENCES=p4
endif

## Own configuration directives
#

CONFIG_ACPI = y

## Accepted configuration directives
#

ifeq ($(CONFIG_SMP),y)
	DEFS += -DCONFIG_SMP
endif
ifeq ($(CONFIG_HT),y)
	DEFS += -DCONFIG_HT
endif
ifeq ($(CONFIG_FPU_LAZY),y)
	DEFS += -DCONFIG_FPU_LAZY
endif

ARCH_SOURCES = \
	arch/$(ARCH)/src/dummy.s \
	arch/$(ARCH)/src/fpu_context.c \
	arch/$(ARCH)/src/boot/boot.S \
	arch/$(ARCH)/src/boot/memmap.S \
	arch/$(ARCH)/src/pm.c \
	arch/$(ARCH)/src/context.S \
	arch/$(ARCH)/src/drivers/ega.c \
	arch/$(ARCH)/src/drivers/i8042.c \
	arch/$(ARCH)/src/drivers/i8254.c \
	arch/$(ARCH)/src/drivers/i8259.c \
	arch/$(ARCH)/src/delay.S \
	arch/$(ARCH)/src/amd64.c \
	arch/$(ARCH)/src/bios/bios.c \
	arch/$(ARCH)/src/interrupt.c \
	arch/$(ARCH)/src/mm/frame.c \
	arch/$(ARCH)/src/mm/page.c \
	arch/$(ARCH)/src/mm/tlb.c \
	arch/$(ARCH)/src/asm_utils.S \
	arch/$(ARCH)/src/fmath.c \
	arch/$(ARCH)/src/mm/memory_init.c \
	arch/$(ARCH)/src/cpu/cpu.c \
	arch/$(ARCH)/src/proc/scheduler.c \
	arch/$(ARCH)/src/userspace.c

ifeq ($(CONFIG_SMP),y)
	ARCH_SOURCES += \
		arch/$(ARCH)/src/smp/ap.S \
		arch/$(ARCH)/src/smp/apic.c \
		arch/$(ARCH)/src/smp/ipi.c \
		arch/$(ARCH)/src/smp/mps.c \
		arch/$(ARCH)/src/smp/smp.c
endif
