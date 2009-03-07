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

BFD_NAME = elf64-sparc
BFD_ARCH = sparc
BFD = binary
TARGET = sparc64-linux-gnu
TOOLCHAIN_DIR = $(CROSS_PREFIX)/sparc64

GCC_CFLAGS += -m64 -mcpu=ultrasparc 
SUNCC_CFLAGS += -m64 -xarch=sparc -xregs=appl,no%float

LFLAGS += -no-check-sections -N

DEFS += -D__64_BITS__

ifeq ($(PROCESSOR),us)
	DEFS += -DUS
endif

ifeq ($(PROCESSOR),us3)
	DEFS += -DUS3
endif

ARCH_SOURCES = \
	arch/$(KARCH)/src/cpu/cpu.c \
	arch/$(KARCH)/src/asm.S \
	arch/$(KARCH)/src/panic.S \
	arch/$(KARCH)/src/console.c \
	arch/$(KARCH)/src/context.S \
	arch/$(KARCH)/src/fpu_context.c \
	arch/$(KARCH)/src/dummy.s \
	arch/$(KARCH)/src/mm/as.c \
	arch/$(KARCH)/src/mm/cache.S \
	arch/$(KARCH)/src/mm/frame.c \
	arch/$(KARCH)/src/mm/page.c \
	arch/$(KARCH)/src/mm/tlb.c \
	arch/$(KARCH)/src/sparc64.c \
	arch/$(KARCH)/src/start.S \
	arch/$(KARCH)/src/proc/scheduler.c \
	arch/$(KARCH)/src/proc/thread.c \
	arch/$(KARCH)/src/trap/mmu.S \
	arch/$(KARCH)/src/trap/trap_table.S \
	arch/$(KARCH)/src/trap/trap.c \
	arch/$(KARCH)/src/trap/exception.c \
	arch/$(KARCH)/src/trap/interrupt.c \
	arch/$(KARCH)/src/ddi/ddi.c \
	arch/$(KARCH)/src/drivers/tick.c \
	arch/$(KARCH)/src/drivers/kbd.c \
	arch/$(KARCH)/src/drivers/sgcn.c \
	arch/$(KARCH)/src/drivers/pci.c

ifeq ($(CONFIG_FB),y)
	ARCH_SOURCES += \
		arch/$(KARCH)/src/drivers/scr.c
endif

ifeq ($(CONFIG_SMP),y)
	ARCH_SOURCES += \
		arch/$(KARCH)/src/smp/ipi.c \
		arch/$(KARCH)/src/smp/smp.c
endif

ifeq ($(CONFIG_TSB),y)
	ARCH_SOURCES += \
		arch/$(KARCH)/src/mm/tsb.c
endif

ifdef CONFIG_Z8530
	ARCH_SOURCES += \
		arch/$(KARCH)/src/drivers/fhc.c
endif
