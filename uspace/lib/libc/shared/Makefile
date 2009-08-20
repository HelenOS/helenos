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

include ../../../../version

## Setup toolchain
#

LIBC_PREFIX = ../../../lib/libc
APP_LIBC_PREFIX = ../../lib/libc
SOFTINT_PREFIX = ../../../lib/softint

include $(LIBC_PREFIX)/Makefile.toolchain
#include arch/$(UARCH)/Makefile.inc
ARCH_SOURCES :=

LFLAGS = -shared -soname libc.so.0 --whole-archive 

LIBS = \
	$(LIBC_PREFIX)/libc.pic.a \
	$(SOFTINT_PREFIX)/libsoftint.pic.a

DEFS += -DRELEASE=\"$(RELEASE)\"

## Sources
#

OUTPUT = libc.so.0

.PHONY: all clean

all: $(OUTPUT) arch/$(UARCH)/_link.ld

inc:
	ln -sfn ../arch/$(UARCH)/include include/arch

-include Makefile.depend

clean:
	-rm -f $(OUTPUT) $(OUTPUT).map $(OUTPUT).disasm $(OUTPUT).sections Makefile.depend *.o arch/$(UARCH)/_link.ld  arch/$(UARCH)/_lib.ld include/arch
	find arch/$(UARCH)/ -name '*.o' -follow -exec rm \{\} \;

$(OUTPUT): $(LIBS) arch/$(UARCH)/_lib.ld
	$(LD) -T arch/$(UARCH)/_lib.ld $(LFLAGS) $(LIBS) -o $@ -Map $(OUTPUT).map

arch/$(UARCH)/_lib.ld: arch/$(UARCH)/_lib.ld.in
	$(CC) $(DEFS) $(CFLAGS) -DLIBC_PREFIX=$(LIBC_PREFIX) -E -x c $< | grep -v "^\#" > $@

arch/$(UARCH)/_link.ld: arch/$(UARCH)/_link.ld.in
	$(CC) $(DEFS) $(CFLAGS) -DLIBC_PREFIX=$(APP_LIBC_PREFIX) -E -x c $< | grep -v "^\#" > $@
