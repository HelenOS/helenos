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

BFD_NAME = elf32-littlearm
BFD_ARCH = arm
BFD = binary

ATSIGN = %

COMMON_CFLAGS += -fno-omit-frame-pointer -mapcs-frame -march=$(subst _,-,$(PROCESSOR_ARCH)) -mno-unaligned-access -mfpu=vfpv3

ifeq ($(CONFIG_FPU),y)
	# This is necessary to allow vmsr insn and fpexc manipulation
	# Use vfp32 to allow context save/restore of d16-d31 regs.
	AFLAGS += -mfloat-abi=hard -mfpu=vfp3
endif

BITS = 32
ENDIANESS = LE

ARCH_SOURCES = \
	arch/$(KARCH)/src/start.S \
	arch/$(KARCH)/src/asm.S \
	arch/$(KARCH)/src/eabi.S \
	arch/$(KARCH)/src/exc_handler.S \
	arch/$(KARCH)/src/arm32.c \
	arch/$(KARCH)/src/machine_func.c \
	arch/$(KARCH)/src/context.S \
	arch/$(KARCH)/src/dummy.S \
	arch/$(KARCH)/src/cpu/cpu.c \
	arch/$(KARCH)/src/interrupt.c \
	arch/$(KARCH)/src/exception.c \
	arch/$(KARCH)/src/userspace.c \
	arch/$(KARCH)/src/debug/stacktrace.c \
	arch/$(KARCH)/src/debug/stacktrace_asm.S \
	arch/$(KARCH)/src/mm/km.c \
	arch/$(KARCH)/src/mm/as.c \
	arch/$(KARCH)/src/mm/frame.c \
	arch/$(KARCH)/src/mm/page.c \
	arch/$(KARCH)/src/mm/tlb.c \
	arch/$(KARCH)/src/mm/page_fault.c \
	arch/$(KARCH)/src/atomic.c \
	arch/$(KARCH)/src/ras.c

ifeq ($(CONFIG_FPU),y)
	ARCH_SOURCES +=	arch/$(KARCH)/src/fpu_context.c
	ARCH_SOURCES +=	arch/$(KARCH)/src/fpu.S
endif

ifeq ($(MACHINE),gta02)
	ARCH_SOURCES += arch/$(KARCH)/src/mach/gta02/gta02.c
endif

ifeq ($(MACHINE),integratorcp)
	ARCH_SOURCES += arch/$(KARCH)/src/mach/integratorcp/integratorcp.c
endif

ifeq ($(MACHINE),beagleboardxm)
	ARCH_SOURCES += arch/$(KARCH)/src/mach/beagleboardxm/beagleboardxm.c
endif

ifeq ($(MACHINE),beaglebone)
	ARCH_SOURCES += arch/$(KARCH)/src/mach/beaglebone/beaglebone.c
endif

ifeq ($(MACHINE),raspberrypi)
	ARCH_SOURCES += arch/$(KARCH)/src/mach/raspberrypi/raspberrypi.c
endif

ifeq ($(CONFIG_PL050),y)
	ARCH_SOURCES += genarch/src/drivers/pl050/pl050.c
endif
