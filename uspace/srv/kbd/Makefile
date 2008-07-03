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

## Setup toolchain
#

LIBC_PREFIX = ../../lib/libc
SOFTINT_PREFIX = ../../lib/softint
include $(LIBC_PREFIX)/Makefile.toolchain 
include ../../../Makefile.config

CFLAGS += -Iinclude -I../libadt/include

LIBS = $(LIBC_PREFIX)/libc.a

## Sources
#

OUTPUT = kbd
GENERIC_SOURCES = \
	generic/kbd.c \
	generic/key_buffer.c

ARCH_SOURCES = \
	arch/$(ARCH)/src/kbd.c

ifeq ($(ARCH), ia32)
	ARCH_SOURCES += \
		arch/$(ARCH)/src/mouse.c \
		arch/$(ARCH)/src/scanc.c
	GENARCH_SOURCES = \
		genarch/src/kbd.c
	CFLAGS += -DMOUSE_ENABLED
endif
ifeq ($(ARCH), amd64)
	ARCH_SOURCES += \
		arch/$(ARCH)/src/mouse.c \
		arch/$(ARCH)/src/scanc.c
	GENARCH_SOURCES = \
		genarch/src/kbd.c
	CFLAGS += -DMOUSE_ENABLED
endif
ifeq ($(ARCH), sparc64)
	ARCH_SOURCES += \
		arch/$(ARCH)/src/scanc.c
	GENARCH_SOURCES = \
		genarch/src/kbd.c
endif
ifeq ($(ARCH), arm32)
	ARCH_SOURCES += \
		arch/$(ARCH)/src/kbd_gxemul.c
endif


GENERIC_OBJECTS := $(addsuffix .o,$(basename $(GENERIC_SOURCES)))
ARCH_OBJECTS := $(addsuffix .o,$(basename $(ARCH_SOURCES)))
GENARCH_OBJECTS := $(addsuffix .o,$(basename $(GENARCH_SOURCES)))

.PHONY: all clean depend disasm links

all: links $(OUTPUT) $(OUTPUT).disasm

-include Makefile.depend

links:
	ln -sfn ../arch/$(ARCH)/include include/arch
	ln -sfn ../genarch/include include/genarch

clean:
	-rm -f $(OUTPUT) $(OUTPUT).map $(OUTPUT).disasm Makefile.depend include/arch include/genarch

depend:
	$(CC) $(DEFS) $(CFLAGS) -M $(SOURCES) > Makefile.depend

$(OUTPUT): $(ARCH_OBJECTS) $(GENERIC_OBJECTS) $(GENARCH_OBJECTS) $(LIBS)
	$(LD) -T $(LIBC_PREFIX)/arch/$(ARCH)/_link.ld $(GENERIC_OBJECTS) $(ARCH_OBJECTS) $(GENARCH_OBJECTS) $(LIBS) $(LFLAGS) -o $@ -Map $(OUTPUT).map

disasm: $(OUTPUT).disasm

$(OUTPUT).disasm: $(OUTPUT)
	$(OBJDUMP) -d $< >$@

%.o: %.S
	$(CC) $(DEFS) $(AFLAGS) $(CFLAGS) -D__ASM__ -c $< -o $@

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o: %.c
	$(CC) $(DEFS) $(CFLAGS) -c $< -o $@
