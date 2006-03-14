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

## Common compiler flags
#

LIBC_PREFIX = .

## Setup toolchain
#

include $(LIBC_PREFIX)/Makefile.toolchain

## Sources
#

GENERIC_SOURCES = \
	generic/libc.c \
	generic/io/io.c \
	generic/io/print.c

ARCH_SOURCES += \
	arch/$(ARCH)/src/entry.s

GENERIC_OBJECTS := $(addsuffix .o,$(basename $(GENERIC_SOURCES)))
ARCH_OBJECTS := $(addsuffix .o,$(basename $(ARCH_SOURCES)))

.PHONY: all clean depend kerninc

all: kerninc libc.a arch/$(ARCH)/_link.ld

kerninc:
	ln -sf ../../../kernel/generic/include include/kernel
	ln -sf kernel/arch include/arch

-include Makefile.depend

clean:
	-rm -f include/kernel include/arch libc.a arch/$(ARCH)/_link.ld Makefile.depend
	find generic/ arch/$(ARCH)/ -name '*.o' -follow -exec rm \{\} \;

depend:
	$(CC) $(DEFS) $(CFLAGS) -M $(ARCH_SOURCES) $(GENERIC_SOURCES) > Makefile.depend

libc.a: depend $(ARCH_OBJECTS) $(GENERIC_OBJECTS)
	$(AR) rc libc.a $(ARCH_OBJECTS) $(GENERIC_OBJECTS)

arch/$(ARCH)/_link.ld: arch/$(ARCH)/_link.ld.in
	$(CC) $(DEFS) $(CFLAGS) -E -x c $< | grep -v "^\#" > $@

%.o: %.S
	$(CC) $(DEFS) $(AFLAGS) $(CFLAGS) -D__ASM__ -c $< -o $@

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o: %.c
	$(CC) $(DEFS) $(CFLAGS) -c $< -o $@
