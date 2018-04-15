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

BFD_ARCH = mips
BFD = binary
COMMON_CFLAGS += -msoft-float -mno-abicalls -G 0 -fno-zero-initialized-in-bss -mabi=32
AS_PROLOG = .module softfloat;

BITS = 32

## Accepted MACHINEs
#

ifeq ($(MACHINE),msim)
	BFD_NAME = elf32-tradlittlemips
	ENDIANESS = LE
	COMMON_CFLAGS += -march=r4000
endif
ifeq ($(MACHINE),bmalta)
	BFD_NAME = elf32-tradbigmips
	ENDIANESS = BE
	COMMON_CFLAGS += -D__BE__ -march=4kc
endif
ifeq ($(MACHINE),lmalta)
	BFD_NAME = elf32-tradlittlemips
	ENDIANESS = LE
	COMMON_CFLAGS += -march=4kc
endif

ARCH_SOURCES = \
	arch/$(KARCH)/src/start.S \
	arch/$(KARCH)/src/context.S \
	arch/$(KARCH)/src/mips32.c \
	arch/$(KARCH)/src/asm.S \
	arch/$(KARCH)/src/exception.c \
	arch/$(KARCH)/src/interrupt.c \
	arch/$(KARCH)/src/cache.c \
	arch/$(KARCH)/src/debugger.c \
	arch/$(KARCH)/src/cpu/cpu.c \
	arch/$(KARCH)/src/debug/stacktrace.c \
	arch/$(KARCH)/src/debug/stacktrace_asm.S \
	arch/$(KARCH)/src/mm/km.c \
	arch/$(KARCH)/src/mm/frame.c \
	arch/$(KARCH)/src/mm/page.c \
	arch/$(KARCH)/src/mm/tlb.c \
	arch/$(KARCH)/src/mm/as.c \
	arch/$(KARCH)/src/fpu_context.c \
	arch/$(KARCH)/src/smp/smp.c \
	arch/$(KARCH)/src/smp/smp_call.c \
	arch/$(KARCH)/src/machine_func.c

ifeq ($(MACHINE),msim)
	ARCH_SOURCES += \
		arch/$(KARCH)/src/smp/dorder.c
endif

ifeq ($(MACHINE),$(filter lmalta bmalta,$(MACHINE)))
	ARCH_SOURCES += arch/$(KARCH)/src/mach/malta/malta.c
endif

ifeq ($(MACHINE),msim)
	ARCH_SOURCES += arch/$(KARCH)/src/mach/msim/msim.c
endif

ARCH_AUTOCHECK_HEADERS = \
	arch/$(KARCH)/include/arch/istate_struct.h \
	arch/$(KARCH)/include/arch/context_struct.h \
	arch/$(KARCH)/include/arch/fpu_context_struct.h
