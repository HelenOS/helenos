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

CFLAGS += -Iinclude -I../libadt/include

LIBS = $(LIBC_PREFIX)/libc.a

## Sources
#

OUTPUT = kbd
GENERIC_SOURCES = \
	generic/kbd.c \
	generic/key_buffer.c

ARCH_SOURCES = \
	arch/$(UARCH)/src/kbd.c

ifeq ($(UARCH), ia32)
	ARCH_SOURCES += \
		arch/$(UARCH)/src/mouse.c \
		arch/$(UARCH)/src/scanc.c
	GENARCH_SOURCES = \
		genarch/src/kbd.c
	CFLAGS += -DMOUSE_ENABLED
endif
ifeq ($(UARCH), ia64)
	ARCH_SOURCES += \
		arch/$(UARCH)/src/mouse.c \
		arch/$(UARCH)/src/scanc.c \
		arch/$(UARCH)/src/lkbd.c
	GENARCH_SOURCES = \
		genarch/src/kbd.c
	CFLAGS += -DMOUSE_ENABLED
endif
ifeq ($(UARCH), amd64)
	ARCH_SOURCES += \
		arch/$(UARCH)/src/mouse.c \
		arch/$(UARCH)/src/scanc.c
	GENARCH_SOURCES = \
		genarch/src/kbd.c
	CFLAGS += -DMOUSE_ENABLED
endif
ifeq ($(UARCH), sparc64)
	ARCH_SOURCES += \
		arch/$(UARCH)/src/scanc.c \
		arch/$(UARCH)/src/sgcn.c
	GENARCH_SOURCES = \
		genarch/src/kbd.c \
		genarch/src/nofb.c
endif
ifeq ($(UARCH), arm32)
	ARCH_SOURCES += \
		arch/$(UARCH)/src/kbd_gxemul.c
endif
ifeq ($(UARCH), mips32)
	GENARCH_SOURCES += \
	    genarch/src/nofb.c
endif
ifeq ($(UARCH), mips32eb)
	GENARCH_SOURCES += \
	    genarch/src/nofb.c
endif

GENERIC_OBJECTS := $(addsuffix .o,$(basename $(GENERIC_SOURCES)))
ARCH_OBJECTS := $(addsuffix .o,$(basename $(ARCH_SOURCES)))
GENARCH_OBJECTS := $(addsuffix .o,$(basename $(GENARCH_SOURCES)))

OBJECTS := $(ARCH_OBJECTS) $(GENERIC_OBJECTS) $(GENARCH_OBJECTS)

.PHONY: all clean depend disasm links

all: links $(OUTPUT) $(OUTPUT).disasm

-include Makefile.depend

links:
	ln -sfn ../arch/$(UARCH)/include include/arch
	ln -sfn ../genarch/include include/genarch

clean:
	-rm -f $(OUTPUT) $(OUTPUT).map $(OUTPUT).disasm Makefile.depend include/arch include/genarch $(OBJECTS)

depend:
	$(CC) $(DEFS) $(CFLAGS) -M $(SOURCES) > Makefile.depend

$(OUTPUT): $(OBJECTS) $(LIBS)
	$(LD) -T $(LIBC_PREFIX)/arch/$(UARCH)/_link.ld $(OBJECTS) $(LIBS) $(LFLAGS) -o $@ -Map $(OUTPUT).map

disasm: $(OUTPUT).disasm

$(OUTPUT).disasm: $(OUTPUT)
	$(OBJDUMP) -d $< >$@

%.o: %.S
	$(CC) $(DEFS) $(AFLAGS) $(CFLAGS) -D__ASM__ -c $< -o $@

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o: %.c
	$(CC) $(DEFS) $(CFLAGS) -c $< -o $@
