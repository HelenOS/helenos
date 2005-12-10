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

## Kernel release
#

VERSION = 0
PATCHLEVEL = 1
SUBLEVEL = 0
EXTRAVERSION = 
NAME = Dawn
RELEASE = $(VERSION).$(PATCHLEVEL).$(SUBLEVEL)$(EXTRAVERSION)

## Include configuration
#

-include Makefile.config

## Common compiler flags
#

DEFS = -DARCH=$(ARCH) -DRELEASE=\"$(RELEASE)\" "-DNAME=\"$(NAME)\""
CFLAGS = -fno-builtin -fomit-frame-pointer -Werror-implicit-function-declaration -Wmissing-prototypes -Werror -O3 -nostdlib -nostdinc -Igeneric/include/
LFLAGS = -M
AFLAGS =

ifdef REVISION
	DEFS += "-DREVISION=\"$(REVISION)\""
endif

ifdef TIMESTAMP
	DEFS += "-DTIMESTAMP=\"$(TIMESTAMP)\""
endif

## Setup kernel configuration
#

-include arch/$(ARCH)/Makefile.inc
-include genarch/Makefile.inc

ifeq ($(CONFIG_DEBUG),y)
	DEFS += -DCONFIG_DEBUG
endif
ifeq ($(CONFIG_DEBUG_SPINLOCK),y)
	DEFS += -DCONFIG_DEBUG_SPINLOCK
endif
ifeq ($(CONFIG_USERSPACE),y)
	DEFS += -DCONFIG_USERSPACE
endif
ifeq ($(CONFIG_FPU_LAZY),y)
	DEFS += -DCONFIG_FPU_LAZY
endif

## Toolchain configuration
#

ifeq ($(COMPILER),native)
	CC = gcc
	AS = as
	LD = ld
	OBJCOPY = objcopy
	OBJDUMP = objdump
else
	CC = $(TOOLCHAIN_DIR)/$(TARGET)-gcc
	AS = $(TOOLCHAIN_DIR)/$(TARGET)-as
	LD = $(TOOLCHAIN_DIR)/$(TARGET)-ld
	OBJCOPY = $(TOOLCHAIN_DIR)/$(TARGET)-objcopy
	OBJDUMP = $(TOOLCHAIN_DIR)/$(TARGET)-objdump
endif

## Generic kernel sources
#

GENERIC_SOURCES = \
	generic/src/console/chardev.c \
	generic/src/console/console.c \
	generic/src/console/kconsole.c \
	generic/src/cpu/cpu.c \
	generic/src/interrupt/interrupt.c \
	generic/src/main/main.c \
	generic/src/main/kinit.c \
	generic/src/main/uinit.c \
	generic/src/proc/scheduler.c \
	generic/src/proc/thread.c \
	generic/src/proc/task.c \
	generic/src/proc/the.c \
	generic/src/mm/buddy.c \
	generic/src/mm/heap.c \
	generic/src/mm/frame.c \
	generic/src/mm/page.c \
	generic/src/mm/tlb.c \
	generic/src/mm/vm.c \
	generic/src/lib/func.c \
	generic/src/lib/list.c \
	generic/src/lib/memstr.c \
	generic/src/lib/sort.c \
	generic/src/debug/print.c \
	generic/src/debug/symtab.c \
	generic/src/time/clock.c \
	generic/src/time/timeout.c \
	generic/src/time/delay.c \
	generic/src/preempt/preemption.c \
	generic/src/synch/spinlock.c \
	generic/src/synch/condvar.c \
	generic/src/synch/rwlock.c \
	generic/src/synch/mutex.c \
	generic/src/synch/semaphore.c \
	generic/src/synch/waitq.c \
	generic/src/smp/ipi.c \
	generic/src/fb/font-8x16.c

## Test sources
#

ifneq ($(CONFIG_TEST),)
	DEFS += -DCONFIG_TEST
	GENERIC_SOURCES += test/$(CONFIG_TEST)/test.c
endif

GENERIC_OBJECTS := $(addsuffix .o,$(basename $(GENERIC_SOURCES)))
ARCH_OBJECTS := $(addsuffix .o,$(basename $(ARCH_SOURCES)))
GENARCH_OBJECTS := $(addsuffix .o,$(basename $(GENARCH_SOURCES)))

.PHONY: all build clean config links depend boot

all:
	tools/config.py default
	$(MAKE) -C . build

build: kernel.bin boot disasm

config:
	tools/config.py

-include Makefile.depend

distclean: clean
	-rm Makefile.config

clean:
	-rm -f kernel.bin kernel.raw kernel.map kernel.map.pre kernel.objdump kernel.disasm generic/src/debug/real_map.bin Makefile.depend* generic/include/arch generic/include/genarch arch/$(ARCH)/_link.ld
	find generic/src/ arch/*/src/ genarch/src/ test/ -name '*.o' -follow -exec rm \{\} \;
	for arch in arch/*; do \
	    [ -e $$arch/_link.ld ] && rm $$arch/_link.ld 2>/dev/null;\
	    $(MAKE) -C $$arch/boot clean; \
	done;exit 0

archlinks:
	ln -sfn ../../arch/$(ARCH)/include/ generic/include/arch
	ln -sfn ../../genarch/include/ generic/include/genarch

depend: archlinks
	-makedepend $(DEFS) $(CFLAGS) -f - $(ARCH_SOURCES) $(GENARCH_SOURCES) $(GENERIC_SOURCES) >Makefile.depend 2>/dev/null
	#$(CC) $(DEFS) $(CFLAGS) -M $(ARCH_SOURCES) $(GENARCH_SOURCES) $(GENERIC_SOURCES) > Makefile.depend

arch/$(ARCH)/_link.ld: arch/$(ARCH)/_link.ld.in
	$(CC) $(DEFS) $(CFLAGS) -E -x c $< | grep -v "^\#" > $@

generic/src/debug/real_map.bin: depend arch/$(ARCH)/_link.ld $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS)
	$(OBJCOPY) -I binary -O $(BFD_NAME) -B $(BFD_ARCH) --prefix-sections=symtab Makefile generic/src/debug/empty_map.o
	$(LD) -T arch/$(ARCH)/_link.ld $(LFLAGS) $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) generic/src/debug/empty_map.o -o $@ -Map kernel.map.pre
	$(OBJDUMP) -t $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) > kernel.objdump
	tools/genmap.py kernel.map.pre kernel.objdump generic/src/debug/real_map.bin 

generic/src/debug/real_map.o: generic/src/debug/real_map.bin
	$(OBJCOPY) -I binary -O $(BFD_NAME) -B $(BFD_ARCH) --prefix-sections=symtab $< $@

kernel.raw: depend arch/$(ARCH)/_link.ld $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) generic/src/debug/real_map.o
	$(LD) -T arch/$(ARCH)/_link.ld $(LFLAGS) $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(GENERIC_OBJECTS) generic/src/debug/real_map.o -o $@ -Map kernel.map

kernel.bin: kernel.raw
	$(OBJCOPY) -O $(BFD) kernel.raw kernel.bin

boot: kernel.bin
	$(MAKE) -C arch/$(ARCH)/boot build KERNEL_SIZE="`cat kernel.bin | wc -c`" CC=$(CC) AS=$(AS) LD=$(LD)

disasm: kernel.raw
	$(OBJDUMP) -d kernel.raw > kernel.disasm

%.o: %.S
	$(CC) $(DEFS) $(AFLAGS) $(CFLAGS) -D__ASM__ -c $< -o $@

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o: %.c
	$(CC) $(DEFS) $(CFLAGS) -c $< -o $@
