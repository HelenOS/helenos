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

BFD_NAME = elf64-sparc
BFD_ARCH = sparc
BFD = binary

COMMON_CFLAGS += -m64 -mcpu=ultrasparc -mcmodel=medlow -mno-fpu

LFLAGS += -no-check-sections

BITS = 64
ENDIANESS = BE

ifeq ($(PROCESSOR),us)
	DEFS += -DUS
	DEFS += -DSUN4U
	USARCH = sun4u
endif

ifeq ($(PROCESSOR),us3)
	DEFS += -DUS3
	DEFS += -DSUN4U
	USARCH = sun4u
endif

ifeq ($(PROCESSOR),sun4v)
	DEFS += -DSUN4V
	USARCH = sun4v
#MH
	DEFS += -DUS
endif

ARCH_SOURCES = \
	arch/$(KARCH)/src/sparc64.c \
	arch/$(KARCH)/src/cpu/$(USARCH)/cpu.c \
	arch/$(KARCH)/src/debug/stacktrace.c \
	arch/$(KARCH)/src/debug/stacktrace_asm.S \
	arch/$(KARCH)/src/asm.S \
	arch/$(KARCH)/src/$(USARCH)/asm.S \
	arch/$(KARCH)/src/console.c \
	arch/$(KARCH)/src/context.S \
	arch/$(KARCH)/src/fpu_context.c \
	arch/$(KARCH)/src/dummy.S \
	arch/$(KARCH)/src/mm/$(USARCH)/km.c \
	arch/$(KARCH)/src/mm/$(USARCH)/as.c \
	arch/$(KARCH)/src/mm/$(USARCH)/frame.c \
	arch/$(KARCH)/src/mm/page.c \
	arch/$(KARCH)/src/mm/$(USARCH)/tlb.c \
	arch/$(KARCH)/src/$(USARCH)/sparc64.c \
	arch/$(KARCH)/src/$(USARCH)/start.S \
	arch/$(KARCH)/src/proc/$(USARCH)/scheduler.c \
	arch/$(KARCH)/src/proc/thread.c \
	arch/$(KARCH)/src/trap/$(USARCH)/mmu.S \
	arch/$(KARCH)/src/trap/$(USARCH)/trap_table.S \
	arch/$(KARCH)/src/trap/trap.c \
	arch/$(KARCH)/src/trap/exception.c \
	arch/$(KARCH)/src/trap/interrupt.c \
	arch/$(KARCH)/src/drivers/tick.c \
	arch/$(KARCH)/src/drivers/kbd.c \
	arch/$(KARCH)/src/drivers/pci.c \
	arch/$(KARCH)/src/trap/$(USARCH)/interrupt.c

ifeq ($(USARCH),sun4v)
	ARCH_SOURCES += \
		arch/$(KARCH)/src/drivers/niagara.c \
		arch/$(KARCH)/src/sun4v/md.c
endif

ifeq ($(CONFIG_FB),y)
	ARCH_SOURCES += \
		arch/$(KARCH)/src/drivers/scr.c
endif

ifeq ($(CONFIG_SMP),y)
	ARCH_SOURCES += \
		arch/$(KARCH)/src/smp/$(USARCH)/smp.c \
		arch/$(KARCH)/src/smp/$(USARCH)/smp_call.c \
		arch/$(KARCH)/src/smp/$(USARCH)/ipi.c
endif

ifeq ($(CONFIG_TSB),y)
	ARCH_SOURCES += \
		arch/$(KARCH)/src/mm/$(USARCH)/tsb.c
endif

ARCH_AUTOCHECK_HEADERS = \
	arch/$(KARCH)/include/arch/istate_struct.h \
	arch/$(KARCH)/include/arch/context_struct.h

