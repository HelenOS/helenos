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
	genarch/gsp.c \
	genarch/stroke.c \
	generic/key_buffer.c

ARCH_SOURCES =
GENARCH_SOURCES =

ifeq ($(KBD_LAYOUT), us_qwerty)
	GENARCH_SOURCES += layout/us_qwerty.c
endif
ifeq ($(KBD_LAYOUT), us_dvorak)
	GENARCH_SOURCES += layout/us_dvorak.c
endif

ifeq ($(UARCH), amd64)
	GENARCH_SOURCES += \
		port/i8042.c \
		ctl/pc.c
endif
ifeq ($(UARCH), arm32)
	GENARCH_SOURCES += \
		port/gxemul.c
	ifeq ($(CONFIG_FB), y)
		GENARCH_SOURCES += \
			ctl/gxe_fb.c
	else
		GENARCH_SOURCES += \
			ctl/stty.c
	endif
endif
ifeq ($(UARCH), ia32)
	GENARCH_SOURCES += \
		port/i8042.c \
		ctl/pc.c
endif
ifeq ($(MACHINE), i460GX)
	GENARCH_SOURCES += \
		port/i8042.c \
		ctl/pc.c
endif
ifeq ($(MACHINE), ski)
	GENARCH_SOURCES += \
		port/ski.c \
		ctl/stty.c
endif
ifeq ($(MACHINE), msim)
	GENARCH_SOURCES += \
		port/msim.c \
		ctl/stty.c
endif
ifeq ($(MACHINE), lgxemul)
	GENARCH_SOURCES += \
		port/gxemul.c
	ifeq ($(CONFIG_FB), y)
		GENARCH_SOURCES += \
			ctl/gxe_fb.c
	else
		GENARCH_SOURCES += \
			ctl/stty.c
	endif
endif
ifeq ($(MACHINE), bgxemul)
	GENARCH_SOURCES += \
		port/gxemul.c \
		ctl/stty.c
endif
ifeq ($(UARCH), ppc32)
	GENARCH_SOURCES += \
		port/dummy.c \
		ctl/stty.c
endif
ifeq ($(UARCH), sparc64)
	GENARCH_SOURCES += \
		port/z8530.c \
		ctl/sun.c
endif

GENERIC_OBJECTS := $(addsuffix .o,$(basename $(GENERIC_SOURCES)))
ARCH_OBJECTS := $(addsuffix .o,$(basename $(ARCH_SOURCES)))
GENARCH_OBJECTS := $(addsuffix .o,$(basename $(GENARCH_SOURCES)))

OBJECTS := $(ARCH_OBJECTS) $(GENERIC_OBJECTS) $(GENARCH_OBJECTS)

.PHONY: all clean depend disasm links

all: $(OUTPUT) $(OUTPUT).disasm

-include Makefile.depend

clean:
	-rm -f $(OUTPUT) $(OUTPUT).map $(OUTPUT).disasm Makefile.depend $(OBJECTS)

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
