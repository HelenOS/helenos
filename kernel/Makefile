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

## Configuration
#

ROOT_PATH = ..

VERSION_DEF = $(ROOT_PATH)/version

COMMON_MAKEFILE = $(ROOT_PATH)/Makefile.common
COMMON_HEADER = $(ROOT_PATH)/common.h
COMMON_HEADER_ARCH = arch/$(KARCH)/include/common.h

CONFIG_MAKEFILE = $(ROOT_PATH)/Makefile.config
CONFIG_HEADER = $(ROOT_PATH)/config.h

-include $(VERSION_DEF)
-include $(COMMON_MAKEFILE)
-include $(CONFIG_MAKEFILE)

## Common names
#

DEPEND = Makefile.depend
DEPEND_PREV = $(DEPEND).prev
RAW = kernel.raw
BIN = kernel.bin
MAP = kernel.map
JOB = kernel.job
MAP_PREV = $(MAP).prev
DISASM = kernel.disasm
DUMP = kernel.dump
REAL_MAP = generic/src/debug/real_map

ARCH_INCLUDE = generic/include/arch
GENARCH_INCLUDE = generic/include/genarch

GENMAP = tools/genmap.py
JOBFILE = $(ROOT_PATH)/tools/jobfile.py

LINK = arch/$(KARCH)/_link.ld
EMPTY_MAP = generic/src/debug/empty_map.o
SIZEOK_MAP = generic/src/debug/sizeok_map.o

.PHONY: all clean

all: $(VERSION_DEF) $(COMMON_MAKEFILE) $(COMMON_HEADER) $(CONFIG_MAKEFILE) $(CONFIG_HEADER) $(BIN) $(DISASM)
	-[ -f $(DEPEND) ] && cp -a $(DEPEND) $(DEPEND_PREV)

clean:
	rm -f $(DEPEND) $(DEPEND_PREV) $(RAW) $(BIN) $(MAP) $(JOB) $(MAP_PREV) $(DISASM) $(DUMP) $(REAL_MAP).* $(ARCH_INCLUDE) $(GENARCH_INCLUDE) arch/*/_link.ld arch/*/include/common.h
	find generic/src/ arch/*/src/ genarch/src/ test/ -name '*.o' -follow -exec rm \{\} \;

## Common compiler flags
#

INCLUDES = generic/include
OPTIMIZATION = 3

DEFS = -DKERNEL -DRELEASE=$(RELEASE) "-DNAME=$(NAME)" -D__$(BITS)_BITS__ -D__$(ENDIANESS)__

GCC_CFLAGS = -I$(INCLUDES) -O$(OPTIMIZATION) -imacros $(CONFIG_HEADER) \
	-fexec-charset=UTF-8 -fwide-exec-charset=UTF-32$(ENDIANESS) \
	-finput-charset=UTF-8 -ffreestanding -fno-builtin -nostdlib -nostdinc \
	-Wall -Wextra -Wno-unused-parameter -Wmissing-prototypes \
	-Werror-implicit-function-declaration -Wwrite-strings \
	-Werror -pipe

ICC_CFLAGS = -I$(INCLUDES) -O$(OPTIMIZATION) -imacros $(CONFIG_HEADER) \
	-ffreestanding -fno-builtin -nostdlib -nostdinc -Wall -Wmissing-prototypes \
	-Werror-implicit-function-declaration -Werror -wd170

SUNCC_CFLAGS = -I$(INCLUDES) -xO$(OPTIMIZATION) \
	-xnolib -xc99=all -features=extensions \
	-erroff=E_ZERO_SIZED_STRUCT_UNION

CLANG_CFLAGS = -I$(INCLUDES) -O$(OPTIMIZATION) -imacros $(CONFIG_HEADER) \
	-fexec-charset=UTF-8 -fwide-exec-charset=UTF-32$(ENDIANESS) \
	-finput-charset=UTF-8 -ffreestanding -fno-builtin -nostdlib -nostdinc \
	-Wall -Wextra -Wno-unused-parameter -Wmissing-prototypes \
	-Werror-implicit-function-declaration -Wwrite-strings \
	-pipe -arch $(CLANG_ARCH)

-include arch/$(KARCH)/Makefile.inc
-include genarch/Makefile.inc
-include $(DEPEND)

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

SYMTAB_SECTION = ".section symtab.data, \"a\", $(ATSIGN)progbits;"

## Compilation options
#

ifeq ($(COMPILER),gcc_native)
	CFLAGS = $(GCC_CFLAGS)
	DEPEND_DEFS = $(DEFS) $(CONFIG_DEFS)
endif

ifeq ($(COMPILER),gcc_cross)
	CFLAGS = $(GCC_CFLAGS)
	DEPEND_DEFS = $(DEFS) $(CONFIG_DEFS)
endif

ifeq ($(COMPILER),icc)
	CFLAGS = $(ICC_CFLAGS)
	DEPEND_DEFS = $(DEFS) $(CONFIG_DEFS)
endif

ifeq ($(COMPILER),suncc)
	CFLAGS = $(SUNCC_CFLAGS)
	DEFS += $(CONFIG_DEFS)
	DEPEND_DEFS = $(DEFS)
endif

ifeq ($(COMPILER),clang)
	CFLAGS = $(CLANG_CFLAGS)
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
	generic/src/debug/stacktrace.c \
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
	generic/src/lib/str.c \
	generic/src/lib/elf.c \
	generic/src/lib/rd.c \
	generic/src/printf/printf_core.c \
	generic/src/printf/printf.c \
	generic/src/printf/snprintf.c \
	generic/src/printf/vprintf.c \
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
	generic/src/ipc/event.c \
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



$(BIN): $(RAW)
	$(OBJCOPY) -O $(BFD) $< $@

$(DISASM): $(RAW)
	$(OBJDUMP) -d $< > $@

$(RAW): $(LINK) $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(SYMTAB_OBJECTS)
	$(LD) $(LFLAGS) -T $(LINK) -M -Map $(MAP) -o $@  $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(EXTRA_OBJECTS) $(SYMTAB_OBJECTS)

$(LINK): $(LINK).in $(DEPEND)
	$(GCC) $(DEFS) $(GCC_CFLAGS) -D__ASM__ -D__LINKER__ -E -x c $< | grep -v "^\#" > $@

%.o: %.S $(DEPEND)
	$(GCC) $(DEFS) $(GCC_CFLAGS) -D__ASM__ -c $< -o $@
ifeq ($(PRECHECK),y)
	$(JOBFILE) $(JOB) $< $@ as asm/preproc $(DEFS) $(GCC_CFLAGS) -D__ASM__
endif

%.o: %.s $(DEPEND)
	$(AS) $(AFLAGS) -o $@ $<
ifeq ($(PRECHECK),y)
	$(JOBFILE) $(JOB) $< $@ as asm $(DEFS) $(CFLAGS) $(EXTRA_FLAGS)
endif

#
# The FPU tests are the only objects for which we allow the compiler to generate
# FPU instructions.
#

test/fpu/%.o: test/fpu/%.c $(DEPEND)
	$(CC) $(DEFS) $(CFLAGS) $(EXTRA_FLAGS) -c $< -o $@
ifeq ($(PRECHECK),y)
	$(JOBFILE) $(JOB) $< $@ cc test $(DEFS) $(CFLAGS) $(EXTRA_FLAGS)
endif

#
# Ordinary objects.
#

%.o: %.c $(DEPEND)
	$(CC) $(DEFS) $(CFLAGS) $(EXTRA_FLAGS) $(FPU_NO_CFLAGS) -c $< -o $@
ifeq ($(PRECHECK),y)
	$(JOBFILE) $(JOB) $< $@ cc core $(DEFS) $(CFLAGS) $(EXTRA_FLAGS) $(FPU_NO_CFLAGS)
endif

$(REAL_MAP).o: $(REAL_MAP).bin
	echo $(SYMTAB_SECTION)" .incbin \"$<\"" | $(AS) $(AFLAGS) -o $@

$(REAL_MAP).bin: $(LINK) $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS)
	echo $(SYMTAB_SECTION) | $(AS) $(AFLAGS) -o $(EMPTY_MAP)
	$(LD) $(LFLAGS) -T $(LINK) -M -Map $(MAP_PREV) -o $@  $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(EXTRA_OBJECTS) $(EMPTY_MAP)
	$(OBJDUMP) -t $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) > $(DUMP)
	$(GENMAP) $(MAP_PREV) $(DUMP) $@
	
	# Do it once again, this time to get correct even the symbols
	# on architectures that have bss after symtab
	
	echo $(SYMTAB_SECTION)" .incbin \"$@\"" | $(AS) $(AFLAGS) -o $(SIZEOK_MAP)
	$(LD) $(LFLAGS) -T $(LINK) -M -Map $(MAP_PREV) -o $@ $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(EXTRA_OBJECTS) $(SIZEOK_MAP)
	$(OBJDUMP) -t $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) > $(DUMP)
	$(GENMAP) $(MAP_PREV) $(DUMP) $@

$(DEPEND): $(ARCH_INCLUDE) $(GENARCH_INCLUDE) $(COMMON_HEADER_ARCH)
	makedepend -f - -- $(DEPEND_DEFS) $(CFLAGS) -- $(ARCH_SOURCES) $(GENARCH_SOURCES) $(GENERIC_SOURCES) > $@ 2> /dev/null
	-[ -f $(DEPEND_PREV) ] && diff -q $(DEPEND_PREV) $@ && mv -f $(DEPEND_PREV) $@

$(ARCH_INCLUDE): arch/$(KARCH)/include/
	ln -sfn ../../$< $@

$(GENARCH_INCLUDE): genarch/include/
	ln -sfn ../../$< $@

$(COMMON_HEADER_ARCH): $(COMMON_HEADER)
	ln -sfn ../../../$< $@
