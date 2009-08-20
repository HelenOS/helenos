#
# Copyright (c) 2005 Martin Decky
# Copyright (c) 2008 Jiri Svoboda
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

## Common compiler flags
#

LIBC_PREFIX = $(shell pwd)
SOFTINT_PREFIX = ../softint


## Setup toolchain
#

include $(LIBC_PREFIX)/Makefile.toolchain
-include rtld/arch/$(UARCH)/Makefile.inc

CFLAGS += -Irtld/include -I../../srv/loader/include -D__32_BITS__
PIC_CFLAGS := $(CFLAGS) -fPIC -D__IN_SHARED_LIBC__

## Sources
#

GENERIC_SOURCES = \
	generic/libc.c \
	generic/ddi.c \
	generic/as.c \
	generic/cap.c \
	generic/devmap.c \
	generic/event.c \
	generic/errno.c \
	generic/mem.c \
	generic/string.c \
	generic/fibril.c \
	generic/fibril_sync.c \
	generic/pcb.c \
	generic/smc.c \
	generic/thread.c \
	generic/tls.c \
	generic/task.c \
	generic/futex.c \
	generic/io/asprintf.c \
	generic/io/io.c \
	generic/io/printf.c \
	generic/io/klog.c \
	generic/io/snprintf.c \
	generic/io/vprintf.c \
	generic/io/vsnprintf.c \
	generic/io/printf_core.c \
	generic/io/console.c \
	generic/malloc.c \
	generic/sysinfo.c \
	generic/ipc.c \
	generic/async.c \
	generic/dlfcn.c \
	generic/loader.c \
	generic/getopt.c \
	generic/adt/list.o \
	generic/adt/hash_table.o \
	generic/time.c \
	generic/err.c \
	generic/stdlib.c \
	generic/mman.c \
	generic/udebug.c \
	generic/vfs/vfs.c \
	generic/vfs/canonify.c \
	rtld/rtld.c \
	rtld/elf_load.c \
	rtld/dynamic.c \
	rtld/module.c \
	rtld/symbol.c


ARCH_SOURCES += \
	arch/$(UARCH)/src/entry.s \
	arch/$(UARCH)/src/thread_entry.s

GENERIC_OBJECTS := $(addsuffix .o,$(basename $(GENERIC_SOURCES)))
ARCH_OBJECTS := $(addsuffix .o,$(basename $(ARCH_SOURCES)))
OBJECTS := $(GENERIC_OBJECTS) $(ARCH_OBJECTS)
PIC_OBJECTS := $(addsuffix .pio,$(basename $(OBJECTS)))

OUTPUT := libc.a

ifeq ($(CONFIG_BUILD_SHARED_LIBC), y)
	OUTPUT += libc.pic.a
endif

.PHONY: all clean depend kerninc

all: kerninc $(OUTPUT) arch/$(UARCH)/_link.ld

kerninc:
	ln -sfn ../../../../kernel/generic/include include/kernel
	ln -sfn kernel/arch include/arch
	ln -sfn ../arch/$(UARCH)/include include/libarch
	ln -sfn ../arch/$(UARCH)/include rtld/include/arch

-include Makefile.depend

clean:
	-rm -f include/kernel include/arch include/libarch rtld/include/arch libc.a libc.pic.a arch/$(UARCH)/_link.ld Makefile.depend
	find . \( -name '*.o' -o -name '*.pio' \) -follow -exec rm \{\} \;

depend: kerninc
	-makedepend -f - -- $(DEPEND_DEFS) $(CFLAGS) -- $(ARCH_SOURCES) $(GENERIC_SOURCES) > Makefile.depend 2> /dev/null
	-makedepend $(DEFS) $(PIC_CFLAGS) -o.pio -f - $(ARCH_SOURCES) $(GENERIC_SOURCES) >> Makefile.depend 2> /dev/null

libc.a: depend $(OBJECTS)
	$(AR) rc $@ $(LIBS) $(OBJECTS)

libc.pic.a: depend $(PIC_OBJECTS)
	$(AR) rc $@ $(LIBS) $(PIC_OBJECTS)

arch/$(UARCH)/_link.ld: arch/$(UARCH)/_link.ld.in
	$(CC) $(DEFS) $(CFLAGS) -DLIBC_PREFIX=$(LIBC_PREFIX) -E -x c $< | grep -v "^\#" > $@

%.o: %.S
	$(CC) $(DEFS) $(AFLAGS) $(CFLAGS) -D__ASM__ -c $< -o $@

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o: %.c
	$(CC) $(DEFS) $(CFLAGS) -c $< -o $@

%.pio: %.S
	$(CC) $(DEFS) $(AFLAGS) $(CFLAGS) -D__ASM__ -c $< -o $@

%.pio: %.s
	$(AS) $(AFLAGS) $< -o $@

%.pio: %.c
	$(CC) $(DEFS) $(PIC_CFLAGS) -c $< -o $@
