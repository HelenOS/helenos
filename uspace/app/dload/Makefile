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

include ../../../version

## Setup toolchain
#

LIBC_PREFIX = ../../lib/libc
SOFTINT_PREFIX = ../../lib/softint

include $(LIBC_PREFIX)/Makefile.toolchain
include arch/$(UARCH)/Makefile.inc

CFLAGS += -Iinclude -I../../srv/loader/include -I../../lib/libc/rtld/include -O0 -ggdb
LFLAGS +=

LIBS = $(LIBC_PREFIX)/libc.a $(SOFTINT_PREFIX)/libsoftint.a
DEFS += -DRELEASE=\"$(RELEASE)\"

## Sources
#

OUTPUT = dload
GENERIC_SOURCES = \
	dload.c

GENERIC_OBJECTS := $(addsuffix .o,$(basename $(GENERIC_SOURCES)))
ARCH_OBJECTS := $(addsuffix .o,$(basename $(ARCH_SOURCES)))

.PHONY: all clean depend disasm sections inc

all: inc $(OUTPUT) disasm sections

inc:
	ln -sfn ../arch/$(UARCH)/include include/arch

-include Makefile.depend

clean:
	-rm -f $(OUTPUT) $(OUTPUT).map $(OUTPUT).disasm $(OUTPUT).sections Makefile.depend *.o arch/$(UARCH)/_link.ld include/arch
	find arch/$(UARCH)/ -name '*.o' -follow -exec rm \{\} \;

depend:
	$(CC) $(DEFS) $(CFLAGS) -M $(ARCH_SOURCES) $(GENERIC_SOURCES)> Makefile.depend

$(OUTPUT): $(ARCH_OBJECTS) $(GENERIC_OBJECTS) $(LIBS) arch/$(UARCH)/_link.ld
	$(LD) -T arch/$(UARCH)/_link.ld $(ARCH_OBJECTS) $(GENERIC_OBJECTS) $(LIBS) $(LFLAGS) -o $@ -Map $(OUTPUT).map

disasm:
	$(OBJDUMP) -d -S -s $(OUTPUT) >$(OUTPUT).disasm

sections:
	$(OBJDUMP) -h $(OUTPUT) >$(OUTPUT).sections

arch/$(UARCH)/_link.ld: arch/$(UARCH)/_link.ld.in
	$(CC) $(DEFS) $(CFLAGS) -DLIBC_PREFIX=$(LIBC_PREFIX) -E -x c $< | grep -v "^\#" > $@

%.o: %.S
	$(CC) $(DEFS) $(AFLAGS) $(CFLAGS) -D__ASM__ -c $< -o $@

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o: %.c
	$(CC) $(DEFS) $(CFLAGS) -c $< -o $@
