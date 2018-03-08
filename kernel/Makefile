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
COMMON_HEADER_ARCH = arch/$(KARCH)/include/arch/common.h

CONFIG_MAKEFILE = $(ROOT_PATH)/Makefile.config
CONFIG_HEADER = $(ROOT_PATH)/config.h

-include $(VERSION_DEF)
-include $(COMMON_MAKEFILE)
-include $(CONFIG_MAKEFILE)

## Common names
#

AGDEPEND = Makefile.ag.depend
RAW = kernel.raw
BIN = kernel.bin
MAP = kernel.map
JOB = kernel.job
MAP_PREV = $(MAP).prev
DISASM = kernel.disasm
DUMP = kernel.dump
REAL_MAP = generic/src/debug/real_map

GENMAP = tools/genmap.py

LINK = arch/$(KARCH)/_link.ld
EMPTY_MAP = generic/src/debug/empty_map.o
SIZEOK_MAP = generic/src/debug/sizeok_map.o

ifeq ($(PRECHECK),y)
	JOBFILE = $(ROOT_PATH)/tools/jobfile.py
	# XXX: Do not change the order of arguments.
	CC_JOB = $(JOBFILE) $(JOB) $(CC) $< -o $@
else
	CC_JOB = $(CC) $< -o $@
endif

.PHONY: all clean autogen_clean depend

all: $(VERSION_DEF) $(COMMON_MAKEFILE) $(COMMON_HEADER) $(CONFIG_MAKEFILE) $(CONFIG_HEADER) $(BIN) $(DISASM)

clean: autogen_clean
	rm -f $(RAW) $(BIN) $(MAP) $(JOB) $(MAP_PREV) $(DISASM) $(DUMP) $(REAL_MAP).* arch/*/_link.ld arch/*/include/arch/common.h
	find generic/src/ arch/*/src/ genarch/src/ test/ -name '*.o' -follow -exec rm '{}' \;
	find generic/src/ arch/*/src/ genarch/src/ test/ -name '*.d' -follow -exec rm '{}' \;

## Common compiler flags
#

INCLUDES = generic/include genarch/include arch/$(KARCH)/include ../abi/include
INCLUDES_FLAGS = $(addprefix -I,$(INCLUDES))

DEFS = -DKERNEL -DRELEASE=$(RELEASE) "-DCOPYRIGHT=$(COPYRIGHT)" "-DNAME=$(NAME)" -D__$(BITS)_BITS__ -D__$(ENDIANESS)__

COMMON_CFLAGS = $(INCLUDES_FLAGS) -O$(OPTIMIZATION) -imacros $(CONFIG_HEADER) \
	-ffreestanding -nostdlib -nostdinc \
	-fexec-charset=UTF-8 -finput-charset=UTF-8 -fno-common \
	-fdebug-prefix-map=$(realpath $(ROOT_PATH))=.

GCC_CFLAGS = -std=gnu99 -Wall -Wextra -Wno-unused-parameter \
	-Wmissing-prototypes -Werror-implicit-function-declaration \
	-Wwrite-strings -pipe

CLANG_CFLAGS = -std=gnu99 -Wall -Wextra -Wno-unused-parameter \
	-Wno-missing-field-initializers -Wno-unused-command-line-argument \
	-Wmissing-prototypes -Werror-implicit-function-declaration \
	-Wwrite-strings -pipe -fno-stack-protector -fno-PIC

ifeq ($(CONFIG_DEBUG),y)
	COMMON_CFLAGS += -Werror
endif

ifeq ($(CONFIG_LTO),y)
	COMMON_CFLAGS += -flto
endif

ifeq ($(CONFIG_LINE_DEBUG),y)
	COMMON_CFLAGS += -g
endif

#
# Mind the mutual ordering with the inclusion of the arch Makefile.inc.
# AFLAGS and LFLAGS must be initialized before the inclusion.
#
AFLAGS = --fatal-warnings
LFLAGS = -n -T $(LINK) -M --fatal-warnings --warn-common

ifeq ($(CONFIG_STRIP_BINARIES),y)
	LFLAGS += --strip-all
endif

#
# Mind the mutual ordering with the initialization of AFLAGS and LFLAGS.
# The arch Makefile.inc must be included after the initialization.
#
-include arch/$(KARCH)/Makefile.inc
-include genarch/Makefile.inc
-include $(AGDEPEND)

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

SYMTAB_SECTION = .section symtab.data, \"a\", $(ATSIGN)progbits;

## Compilation options
#

ifeq ($(COMPILER),clang)
	CFLAGS = $(COMMON_CFLAGS) $(CLANG_CFLAGS)
else
	CFLAGS = $(COMMON_CFLAGS) $(GCC_CFLAGS)
endif

INSTRUMENTATION = -finstrument-functions

## Generic kernel sources
#

GENERIC_SOURCES = \
	generic/src/adt/avl.c \
	generic/src/adt/bitmap.c \
	generic/src/adt/btree.c \
	generic/src/adt/cht.c \
	generic/src/adt/hash_table.c \
	generic/src/adt/list.c \
	generic/src/console/chardev.c \
	generic/src/console/console.c \
	generic/src/console/prompt.c \
	generic/src/cpu/cpu.c \
	generic/src/cpu/cpu_mask.c \
	generic/src/ddi/ddi.c \
	generic/src/ddi/irq.c \
	generic/src/debug/symtab.c \
	generic/src/debug/stacktrace.c \
	generic/src/debug/panic.c \
	generic/src/debug/debug.c \
	generic/src/interrupt/interrupt.c \
	generic/src/log/log.c \
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
	generic/src/syscall/syscall.c \
	generic/src/syscall/copy.c \
	generic/src/mm/km.c \
	generic/src/mm/reserve.c \
	generic/src/mm/frame.c \
	generic/src/mm/page.c \
	generic/src/mm/tlb.c \
	generic/src/mm/as.c \
	generic/src/mm/backend_anon.c \
	generic/src/mm/backend_elf.c \
	generic/src/mm/backend_phys.c \
	generic/src/mm/backend_user.c \
	generic/src/mm/slab.c \
	generic/src/lib/halt.c \
	generic/src/lib/mem.c \
	generic/src/lib/memfnc.c \
	generic/src/lib/gsort.c \
	generic/src/lib/str.c \
	generic/src/lib/str_error.c \
	generic/src/lib/elf.c \
	generic/src/lib/ra.c \
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
	generic/src/synch/mutex.c \
	generic/src/synch/semaphore.c \
	generic/src/synch/smc.c \
	generic/src/synch/smp_memory_barrier.c \
	generic/src/synch/waitq.c \
	generic/src/synch/futex.c \
	generic/src/synch/workqueue.c \
	generic/src/synch/rcu.c \
	generic/src/smp/ipi.c \
	generic/src/smp/smp.c \
	generic/src/smp/smp_call.c \
	generic/src/ipc/ipc.c \
	generic/src/ipc/sysipc.c \
	generic/src/ipc/sysipc_ops.c \
	generic/src/ipc/ops/conctmeto.c \
	generic/src/ipc/ops/concttome.c \
	generic/src/ipc/ops/dataread.c \
	generic/src/ipc/ops/datawrite.c \
	generic/src/ipc/ops/debug.c \
	generic/src/ipc/ops/pagein.c \
	generic/src/ipc/ops/sharein.c \
	generic/src/ipc/ops/shareout.c \
	generic/src/ipc/ops/stchngath.c \
	generic/src/ipc/ipcrsc.c \
	generic/src/ipc/irq.c \
	generic/src/ipc/event.c \
	generic/src/cap/cap.c \
	generic/src/security/perm.c \
	generic/src/sysinfo/sysinfo.c \
	generic/src/sysinfo/stats.c

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
		test/cht/cht1.c \
		test/avltree/avltree1.c \
		test/fault/fault1.c \
		test/mm/falloc1.c \
		test/mm/falloc2.c \
		test/mm/mapping1.c \
		test/mm/slab1.c \
		test/mm/slab2.c \
		test/synch/semaphore1.c \
		test/synch/semaphore2.c \
		test/synch/workqueue2.c \
		test/synch/workqueue3.c \
		test/synch/rcu1.c \
		test/print/print1.c \
		test/print/print2.c \
		test/print/print3.c \
		test/print/print4.c \
		test/print/print5.c \
		test/thread/thread1.c \
		test/smpcall/smpcall1.c

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

endif

## Sources where instrumentation is enabled
#

ifeq ($(CONFIG_TRACE),y)
	INSTRUMENTED_SOURCES = \
		generic/src/adt/btree.c \
		generic/src/cpu/cpu.c \
		generic/src/ddi/ddi.c \
		generic/src/interrupt/interrupt.c \
		generic/src/main/main.c \
		generic/src/main/kinit.c \
		generic/src/proc/the.c \
		generic/src/mm/frame.c \
		generic/src/mm/page.c \
		generic/src/mm/tlb.c \
		generic/src/mm/as.c \
		generic/src/mm/slab.c \
		generic/src/sysinfo/sysinfo.c \
		generic/src/console/kconsole.c
else
	INSTRUMENTED_SOURCES =
endif

GENERIC_OBJECTS := $(addsuffix .o,$(basename $(GENERIC_SOURCES)))
ARCH_OBJECTS := $(addsuffix .o,$(basename $(ARCH_SOURCES)))
GENARCH_OBJECTS := $(addsuffix .o,$(basename $(GENARCH_SOURCES)))

GENERIC_DEPENDS := $(addsuffix .d,$(basename $(GENERIC_SOURCES)))
ARCH_DEPENDS := $(addsuffix .d,$(basename $(ARCH_SOURCES)))
GENARCH_DEPENDS := $(addsuffix .d,$(basename $(GENARCH_SOURCES)))

GENARCH_AUTOGENS_H := $(addsuffix .h,$(basename $(GENARCH_AUTOGENS_AG)))
GENARCH_AUTOGENS_PROBE_C := $(addsuffix .ag.probe.c,$(basename $(GENARCH_AUTOGENS_AG)))
GENARCH_AUTOGENS_PROBE_S := $(addsuffix .ag.probe.s,$(basename $(GENARCH_AUTOGENS_AG)))

ARCH_AUTOGENS_H := $(addsuffix .h,$(basename $(ARCH_AUTOGENS_AG)))
ARCH_AUTOGENS_PROBE_C := $(addsuffix .ag.probe.c,$(basename $(ARCH_AUTOGENS_AG)))
ARCH_AUTOGENS_PROBE_S := $(addsuffix .ag.probe.s,$(basename $(ARCH_AUTOGENS_AG)))

AUTOGENS_H := $(ARCH_AUTOGENS_H) $(GENARCH_AUTOGENS_H)
AUTOGENS_AG := $(ARCH_AUTOGENS_AG) $(GENARCH_AUTOGENS_AG)

AS_CFLAGS := $(addprefix -Xassembler ,$(AFLAGS))
LD_CFLAGS := $(addprefix -Xlinker ,$(LFLAGS))

-include $(GENERIC_DEPENDS)
-include $(ARCH_DEPENDS)
-include $(GENARCH_DEPENDS)

ifeq ($(COMPILER),clang)
	AS_CFLAGS += -mllvm -asm-macro-max-nesting-depth=1000
endif

ifeq ($(CONFIG_SYMTAB),y)
	SYMTAB_OBJECTS := generic/src/debug/real_map.o
else
	SYMTAB_OBJECTS :=
endif

$(BIN): $(RAW)
	$(OBJCOPY) -O $(BFD) $< $@

$(DISASM): $(RAW)
ifeq ($(CONFIG_LINE_DEBUG),y)
	$(OBJDUMP) -d -S $< > $@
else
	$(OBJDUMP) -d $< > $@
endif

$(RAW): $(LINK) $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(SYMTAB_OBJECTS)
ifeq ($(CONFIG_LTO),y)
	$(CC) $(DEFS) $(CFLAGS) $(LD_CFLAGS) -Xlinker -Map -Xlinker $(MAP) -o $@ $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(EXTRA_OBJECTS) $(SYMTAB_OBJECTS)
else
	$(LD) $(LFLAGS) -Map $(MAP) -o $@ $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(EXTRA_OBJECTS) $(SYMTAB_OBJECTS)
endif

$(LINK): $(LINK).in | depend
	$(CC) $(DEFS) $(CFLAGS) -D__ASSEMBLER__ -D__LINKER__ -E -x c $< | grep -v "^\#" > $@

%.h: %.ag
	$(AUTOGEN) probe $< >$<.probe.c
	$(CC_AUTOGEN) $(DEFS) $(CFLAGS) -S -o $<.probe.s $<.probe.c
	$(AUTOGEN) generate $< <$<.probe.s >$@

%.o: %.S | depend
	$(CC_JOB) -c -MD -MP $(DEFS) $(CFLAGS) $(AS_CFLAGS)

%.o: %.s | depend
	$(CC_JOB) -c -MD -MP $(DEFS) $(CFLAGS) $(AS_CFLAGS)

%.o: %.c | depend
	$(CC_JOB) -c -MD -MP $(DEFS) $(CFLAGS) $(EXTRA_FLAGS) $(FPU_NO_CFLAGS) $(if $(findstring $<,$(INSTRUMENTED_SOURCES)),$(INSTRUMENTATION))

$(REAL_MAP).o: $(REAL_MAP).bin
	echo "$(AS_PROLOG)""$(SYMTAB_SECTION)"" .incbin \"$<\"" | $(CC) $(CFLAGS) $(AS_CFLAGS) -x assembler -c -o $@ -

$(REAL_MAP).bin: $(LINK) $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS)
	echo "$(AS_PROLOG)""$(SYMTAB_SECTION)" | $(CC) $(CFLAGS) $(AS_CFLAGS) -x assembler -c -o $(EMPTY_MAP) -
ifeq ($(CONFIG_LTO),y)
	$(CC) $(DEFS) $(CFLAGS) $(LD_CFLAGS) -Xlinker -Map -Xlinker $(MAP_PREV) -o $@ $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(EXTRA_OBJECTS) $(EMPTY_MAP)
else
	$(LD) $(LFLAGS) -Map $(MAP_PREV) -o $@ $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(EXTRA_OBJECTS) $(EMPTY_MAP)
endif
	$(OBJDUMP) -t $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) > $(DUMP)
	$(GENMAP) $(MAP_PREV) $(DUMP) $@

	# Do it once again, this time to get correct even the symbols
	# on architectures that have bss after symtab

	echo "$(AS_PROLOG)""$(SYMTAB_SECTION)"" .incbin \"$@\"" | $(CC) $(CFLAGS) $(AS_CFLAGS) -x assembler -c -o $(SIZEOK_MAP) -
ifeq ($(CONFIG_LTO),y)
	$(CC) $(DEFS) $(CFLAGS) $(LD_CFLAGS) -Xlinker -Map -Xlinker $(MAP_PREV) -o $@ $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(EXTRA_OBJECTS) $(SIZEOK_MAP)
else
	$(LD) $(LFLAGS) -Map $(MAP_PREV) -o $@ $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) $(EXTRA_OBJECTS) $(SIZEOK_MAP)
endif
	$(OBJDUMP) -t $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) > $(DUMP)
	$(GENMAP) $(MAP_PREV) $(DUMP) $@

$(AUTOGENS_H): $(AGDEPEND)

$(AGDEPEND): $(AUTOGENS_AG)
	echo "# DO NOT EDIT" >$@
	for g in $^; do \
		h=`dirname $$g`/`basename $$g .ag`.h; \
		for l in `$(AUTOGEN) depend $$g`; do \
			echo "$$h: $$l" >>$@; \
		done \
	done

depend: $(AUTOGENS_H)

autogen_clean:
	-rm $(ARCH_AUTOGENS_H) $(ARCH_AUTOGENS_PROBE_C) $(ARCH_AUTOGENS_PROBE_S)
	-rm $(GENARCH_AUTOGENS_H) $(GENARCH_AUTOGENS_PROBE_C) $(GENARCH_AUTOGENS_PROBE_S)
	-rm $(AGDEPEND)
