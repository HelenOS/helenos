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


## Include configuration
#

include ../version
-include ../Makefile.config
-include ../config.defs

INCLUDES = generic/include
OPTIMIZATION = 3

ifndef CROSS_PREFIX
	CROSS_PREFIX = /usr/local
endif

## Common compiler flags
#

DEFS = -DKERNEL -DRELEASE=$(RELEASE) "-DNAME=$(NAME)"

GCC_CFLAGS = -I$(INCLUDES) -O$(OPTIMIZATION) -imacros ../config.h \
	-fexec-charset=UTF-8 -fwide-exec-charset=UTF-32 -finput-charset=UTF-8 \
	-fno-builtin -Wall -Wextra -Wno-unused-parameter -Wmissing-prototypes -Werror \
	-nostdlib -nostdinc -pipe

ICC_CFLAGS = -I$(INCLUDES) -O$(OPTIMIZATION) -imacros ../config.h \
	-fno-builtin -Wall -Wmissing-prototypes -Werror \
	-nostdlib -nostdinc \
	-wd170

SUNCC_CFLAGS = -I$(INCLUDES) -xO$(OPTIMIZATION) \
	-xnolib -xc99=all -features=extensions \
	-erroff=E_ZERO_SIZED_STRUCT_UNION

LFLAGS = -M
AFLAGS =

-include arch/$(KARCH)/Makefile.inc
-include genarch/Makefile.inc

## The at-sign
#
# The $(ATSIGN) variable holds the ASCII character representing the at-sign
# ('@') used in various $(AS) constructs (e.g. @progbits). On architectures that
# don't use '@' for starting a comment, $(ATSIGN) is merely '@'. However, on
# those that do use it for starting a comment (e.g. arm32), $(ATSIGN) must be
# defined as the percentile-sign ('%') in the architecture-dependent
# Makefile.inc.
#
ATSIGN ?= @

## Cross-platform assembly to start a symtab.data section
#
SYMTAB_SECTION=".section symtab.data, \"a\", $(ATSIGN)progbits;"

## Simple detection for the type of the host system
#
HOST = $(shell uname)

## On Solaris, some utilities have slightly different names
#
ifeq ($(HOST),SunOS)
	BINUTILS_PREFIX = "g"
else
	BINUTILS_PREFIX = ""
endif

## Toolchain configuration
#

ifeq ($(COMPILER),gcc_native)
	CC = gcc
	GCC = gcc
	AS = $(BINUTILS_PREFIX)as
	LD = $(BINUTILS_PREFIX)ld
	OBJCOPY = $(BINUTILS_PREFIX)objcopy
	OBJDUMP = $(BINUTILS_PREFIX)objdump
	LIBDIR = /usr/lib
	CFLAGS = $(GCC_CFLAGS)
	DEPEND_DEFS = $(DEFS) $(CONFIG_DEFS)
endif

ifeq ($(COMPILER),icc_native)
	CC = icc
	GCC = gcc
	AS = as
	LD = ld
	OBJCOPY = objcopy
	OBJDUMP = objdump
	LIBDIR = /usr/lib
	CFLAGS = $(ICC_CFLAGS)
	DEPEND_DEFS = $(DEFS) $(CONFIG_DEFS)
endif

ifeq ($(COMPILER),suncc_native)
	CC = suncc
	GCC = gcc
	AS = $(BINUTILS_PREFIX)as
	LD = $(BINUTILS_PREFIX)ld
	OBJCOPY = $(BINUTILS_PREFIX)objcopy
	OBJDUMP = $(BINUTILS_PREFIX)objdump
	LIBDIR = /usr/lib
	CFLAGS = $(SUNCC_CFLAGS)
	DEFS += $(CONFIG_DEFS)
	DEPEND_DEFS = $(DEFS)
endif

ifeq ($(COMPILER),gcc_cross)
	CC = $(TOOLCHAIN_DIR)/bin/$(TARGET)-gcc
	GCC = $(CC)
	AS = $(TOOLCHAIN_DIR)/bin/$(TARGET)-as
	LD = $(TOOLCHAIN_DIR)/bin/$(TARGET)-ld
	OBJCOPY = $(TOOLCHAIN_DIR)/bin/$(TARGET)-objcopy
	OBJDUMP = $(TOOLCHAIN_DIR)/bin/$(TARGET)-objdump
	LIBDIR = $(TOOLCHAIN_DIR)/lib
	CFLAGS = $(GCC_CFLAGS)
	DEPEND_DEFS = $(DEFS) $(CONFIG_DEFS)
endif

## Generic kernel sources
#

GENERIC_SOURCES = \
	generic/src/adt/avl.c \
	generic/src/adt/bitmap.c \
	generic/src/adt/btree.c \
	generic/src/adt/hash_table.c \
	generic/src/adt/list.c \
	generic/src/console/chardev.c \
	generic/src/console/console.c \
	generic/src/cpu/cpu.c \
	generic/src/ddi/ddi.c \
	generic/src/ddi/irq.c \
	generic/src/ddi/device.c \
	generic/src/debug/symtab.c \
	generic/src/event/event.c \
	generic/src/interrupt/interrupt.c \
	generic/src/main/main.c \
	generic/src/main/kinit.c \
	generic/src/main/uinit.c \
	generic/src/main/version.c \
	generic/src/main/shutdown.c \
	generic/src/proc/program.c \
	generic/src/proc/scheduler.c \
	generic/src/proc/thread.c \
	generic/src/proc/task.c \
	generic/src/proc/the.c \
	generic/src/proc/tasklet.c \
	generic/src/syscall/syscall.c \
	generic/src/syscall/copy.c \
	generic/src/mm/buddy.c \
	generic/src/mm/frame.c \
	generic/src/mm/page.c \
	generic/src/mm/tlb.c \
	generic/src/mm/as.c \
	generic/src/mm/backend_anon.c \
	generic/src/mm/backend_elf.c \
	generic/src/mm/backend_phys.c \
	generic/src/mm/slab.c \
	generic/src/lib/func.c \
	generic/src/lib/memstr.c \
	generic/src/lib/sort.c \
	generic/src/lib/string.c \
	generic/src/lib/elf.c \
	generic/src/lib/rd.c \
	generic/src/printf/printf_core.c \
	generic/src/printf/printf.c \
	generic/src/printf/sprintf.c \
	generic/src/printf/snprintf.c \
	generic/src/printf/vprintf.c \
	generic/src/printf/vsprintf.c \
	generic/src/printf/vsnprintf.c \
	generic/src/time/clock.c \
	generic/src/time/timeout.c \
	generic/src/time/delay.c \
	generic/src/preempt/preemption.c \
	generic/src/synch/spinlock.c \
	generic/src/synch/condvar.c \
	generic/src/synch/rwlock.c \
	generic/src/synch/mutex.c \
	generic/src/synch/semaphore.c \
	generic/src/synch/smc.c \
	generic/src/synch/waitq.c \
	generic/src/synch/futex.c \
	generic/src/smp/ipi.c \
	generic/src/smp/smp.c \
	generic/src/ipc/ipc.c \
	generic/src/ipc/sysipc.c \
	generic/src/ipc/ipcrsc.c \
	generic/src/ipc/irq.c \
	generic/src/security/cap.c \
	generic/src/sysinfo/sysinfo.c

## Kernel console support
#

ifeq ($(CONFIG_KCONSOLE),y)
GENERIC_SOURCES += \
	generic/src/console/kconsole.c \
	generic/src/console/cmd.c
endif

## Udebug interface sources
#

ifeq ($(CONFIG_UDEBUG),y)
GENERIC_SOURCES += \
	generic/src/ipc/kbox.c \
	generic/src/udebug/udebug.c \
	generic/src/udebug/udebug_ops.c \
	generic/src/udebug/udebug_ipc.c
endif

## Test sources
#

ifeq ($(CONFIG_TEST),y)
	CFLAGS += -Itest/
	GENERIC_SOURCES += \
		test/test.c \
		test/atomic/atomic1.c \
		test/btree/btree1.c \
		test/avltree/avltree1.c \
		test/fault/fault1.c \
		test/mm/falloc1.c \
		test/mm/falloc2.c \
		test/mm/mapping1.c \
		test/mm/slab1.c \
		test/mm/slab2.c \
		test/synch/rwlock1.c \
		test/synch/rwlock2.c \
		test/synch/rwlock3.c \
		test/synch/rwlock4.c \
		test/synch/rwlock5.c \
		test/synch/semaphore1.c \
		test/synch/semaphore2.c \
		test/print/print1.c \
		test/print/print2.c \
		test/print/print3.c \
		test/print/print4.c \
		test/thread/thread1.c \
		test/sysinfo/sysinfo1.c
	
	ifeq ($(KARCH),mips32)
		GENERIC_SOURCES += test/debug/mips1.c
	else
		GENERIC_SOURCES += test/debug/mips1_skip.c
	endif
	
	ifeq ($(KARCH),ia64)
		GENERIC_SOURCES += test/mm/purge1.c
	else
		GENERIC_SOURCES += test/mm/purge1_skip.c
	endif
	
	ifeq ($(CONFIG_FPU),y)
		ifeq ($(KARCH),ia32)
			TEST_FPU1 = y
			TEST_SSE1 = y
			GENERIC_SOURCES += test/fpu/fpu1_x86.c
		endif
		
		ifeq ($(KARCH),amd64)
			TEST_FPU1 = y
			TEST_SSE1 = y
			GENERIC_SOURCES += test/fpu/fpu1_x86.c
		endif
		
		ifeq ($(KARCH),ia64)
			TEST_FPU1 = y
			GENERIC_SOURCES += test/fpu/fpu1_ia64.c
		endif
		
		ifeq ($(KARCH),mips32)
			TEST_MIPS2 = y
		endif
	endif
	
	ifneq ($(TEST_FPU1),y)
		GENERIC_SOURCES += test/fpu/fpu1_skip.c
	endif
	
	ifeq ($(TEST_SSE1),y)
		GENERIC_SOURCES += test/fpu/sse1.c
	else
		GENERIC_SOURCES += test/fpu/sse1_skip.c
	endif
	
	ifeq ($(TEST_MIPS2),y)
		GENERIC_SOURCES += test/fpu/mips2.c
	else
		GENERIC_SOURCES += test/fpu/mips2_skip.c
	endif
	
endif

GENERIC_OBJECTS := $(addsuffix .o,$(basename $(GENERIC_SOURCES)))
ARCH_OBJECTS := $(addsuffix .o,$(basename $(ARCH_SOURCES)))
GENARCH_OBJECTS := $(addsuffix .o,$(basename $(GENARCH_SOURCES)))

ifeq ($(CONFIG_SYMTAB),y)
	SYMTAB_OBJECTS := generic/src/debug/real_map.o
else
	SYMTAB_OBJECTS :=
endif

.PHONY: all build clean archlinks depend disasm

all: ../Makefile.config ../config.h ../config.defs
	-rm Makefile.depend
	$(MAKE) -C . build

build: kernel.bin disasm

-include Makefile.depend

clean:
	-rm -f kernel.bin kernel.raw kernel.map kernel.map.pre kernel.objdump kernel.disasm generic/src/debug/real_map.bin Makefile.depend* generic/include/arch generic/include/genarch arch/$(KARCH)/_link.ld
	find generic/src/ arch/*/src/ genarch/src/ test/ -name '*.o' -follow -exec rm \{\} \;
	for arch in arch/* ; do \
	    [ -e $$arch/_link.ld ] && rm $$arch/_link.ld 2>/dev/null ; \
	done ; exit 0

archlinks:
	ln -sfn ../../arch/$(KARCH)/include/ generic/include/arch
	ln -sfn ../../genarch/include/ generic/include/genarch

depend: archlinks
	-makedepend -f - -- $(DEPEND_DEFS) $(CFLAGS) -- $(ARCH_SOURCES) $(GENARCH_SOURCES) $(GENERIC_SOURCES) > Makefile.depend 2> /dev/null

arch/$(KARCH)/_link.ld: arch/$(KARCH)/_link.ld.in
	$(GCC) $(DEFS) $(GCC_CFLAGS) -D__ASM__ -D__LINKER__ -E -x c $< | grep -v "^\#" > $@

generic/src/debug/real_map.bin: depend arch/$(KARCH)/_link.ld $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS)
	echo $(SYMTAB_SECTION) | $(AS) $(AFLAGS) -o generic/src/debug/empty_map.o
	$(LD) -T arch/$(KARCH)/_link.ld $(LFLAGS) $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(EXTRA_OBJECTS) generic/src/debug/empty_map.o  -o $@ -Map kernel.map.pre
	$(OBJDUMP) -t $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) > kernel.objdump
	tools/genmap.py kernel.map.pre kernel.objdump generic/src/debug/real_map.bin 
	# Do it once again, this time to get correct even the symbols
	# on architectures, that have bss after symtab
	echo $(SYMTAB_SECTION)" .incbin \"$@\"" | $(AS) $(AFLAGS) -o generic/src/debug/sizeok_map.o
	$(LD) -T arch/$(KARCH)/_link.ld $(LFLAGS) $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(EXTRA_OBJECTS) generic/src/debug/sizeok_map.o -o $@ -Map kernel.map.pre
	$(OBJDUMP) -t $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) > kernel.objdump
	tools/genmap.py kernel.map.pre kernel.objdump generic/src/debug/real_map.bin 

generic/src/debug/real_map.o: generic/src/debug/real_map.bin
	echo $(SYMTAB_SECTION)" .incbin \"$<\"" | $(AS) $(AFLAGS) -o $@ 

kernel.raw: depend arch/$(KARCH)/_link.ld $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(SYMTAB_OBJECTS)
	$(LD) -T arch/$(KARCH)/_link.ld $(LFLAGS) $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(EXTRA_OBJECTS) $(SYMTAB_OBJECTS) -o $@ -Map kernel.map

kernel.bin: kernel.raw
	$(OBJCOPY) -O $(BFD) kernel.raw kernel.bin

disasm: kernel.raw
	$(OBJDUMP) -d kernel.raw > kernel.disasm

%.o: %.S
	$(GCC) $(DEFS) $(GCC_CFLAGS) -D__ASM__ -c $< -o $@

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

#
# The FPU tests are the only objects for which we allow the compiler to generate
# FPU instructions.
#
test/fpu/%.o: test/fpu/%.c
	$(CC) $(DEFS) $(CFLAGS) $(EXTRA_FLAGS) -c $< -o $@

#
# Ordinary objects.
#
%.o: %.c
	$(CC) $(DEFS) $(CFLAGS) $(EXTRA_FLAGS) $(FPU_NO_CFLAGS) -c $< -o $@ 
