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

BFD_NAME = elf64-x86-64
BFD_ARCH = i386:x86-64
BFD = binary

FPU_NO_CFLAGS = -mno-sse -mno-sse2
COMMON_CFLAGS += -mcmodel=$(MEMORY_MODEL) -mno-red-zone -fno-unwind-tables -fno-omit-frame-pointer

BITS = 64
ENDIANESS = LE

## Accepted CPUs
#

ifeq ($(PROCESSOR),opteron)
	COMMON_CFLAGS += -march=opteron
endif

ARCH_SOURCES = \
	arch/$(KARCH)/src/fpu_context.c \
	arch/$(KARCH)/src/boot/multiboot.S \
	arch/$(KARCH)/src/boot/multiboot2.S \
	arch/$(KARCH)/src/boot/memmap.c \
	arch/$(KARCH)/src/debug/stacktrace.c \
	arch/$(KARCH)/src/debug/stacktrace_asm.S \
	arch/$(KARCH)/src/pm.c \
	arch/$(KARCH)/src/vreg.c \
	arch/$(KARCH)/src/kseg.c \
	arch/$(KARCH)/src/context.S \
	arch/$(KARCH)/src/ddi/ddi.c \
	arch/$(KARCH)/src/drivers/i8254.c \
	arch/$(KARCH)/src/drivers/i8259.c \
	arch/$(KARCH)/src/delay.S \
	arch/$(KARCH)/src/amd64.c \
	arch/$(KARCH)/src/bios/bios.c \
	arch/$(KARCH)/src/interrupt.c \
	arch/$(KARCH)/src/mm/km.c \
	arch/$(KARCH)/src/mm/as.c \
	arch/$(KARCH)/src/mm/frame.c \
	arch/$(KARCH)/src/mm/page.c \
	arch/$(KARCH)/src/mm/tlb.c \
	arch/$(KARCH)/src/asm.S \
	arch/$(KARCH)/src/cpu/cpu.c \
	arch/$(KARCH)/src/proc/scheduler.c \
	arch/$(KARCH)/src/proc/task.c \
	arch/$(KARCH)/src/proc/thread.c \
	arch/$(KARCH)/src/userspace.c \
	arch/$(KARCH)/src/syscall.c

ifeq ($(CONFIG_SMP),y)
	ARCH_SOURCES += \
		arch/$(KARCH)/src/smp/ap.S \
		arch/$(KARCH)/src/smp/apic.c \
		arch/$(KARCH)/src/smp/ipi.c \
		arch/$(KARCH)/src/smp/mps.c \
		arch/$(KARCH)/src/smp/smp_call.c \
		arch/$(KARCH)/src/smp/smp.c
endif

ARCH_AUTOGENS_AG = \
	arch/$(KARCH)/include/arch/istate_struct.ag \
	arch/$(KARCH)/include/arch/context_struct.ag \
	arch/$(KARCH)/include/arch/kseg_struct.ag \
	arch/$(KARCH)/include/arch/boot/memmap_struct.ag
