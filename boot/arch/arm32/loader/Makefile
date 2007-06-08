#
# Copyright (c) 2006 Martin Decky
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
include ../../../../Makefile.config

## Toolchain configuration
#

TARGET = arm-linux-gnu
TOOLCHAIN_DIR = /usr/local/arm/bin

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

CFLAGS = -DRELEASE=\"$(RELEASE)\" -I. -I../../../generic -I../../.. -nostdinc -nostdlib -fno-builtin -Werror-implicit-function-declaration -Wmissing-prototypes -Werror -O3 

ifdef REVISION
	CFLAGS += "-DREVISION=\"$(REVISION)\""
endif

ifdef TIMESTAMP
	CFLAGS += "-DTIMESTAMP=\"$(TIMESTAMP)\""
endif

ifdef MACHINE
	CFLAGS += "-DMACHINE=$(MACHINE)"
endif

SOURCES = \
	main.c \
	boot.S \
	asm.S \
	mm.c \
	../../../generic/printf.c \
	../../../genarch/division.c

ifeq ($(MACHINE), gxemul_testarm)
	SOURCES += print/gxemul.c
endif


COMPONENTS = \
	$(KERNELDIR)/kernel.bin \
	$(USPACEDIR)/ns/ns \
	$(USPACEDIR)/init/init \
	$(USPACEDIR)/kbd/kbd \
	$(USPACEDIR)/console/console \
	$(USPACEDIR)/fb/fb \
	$(USPACEDIR)/tester/tester \
	$(USPACEDIR)/klog/klog \
	$(USPACEDIR)/tetris/tetris

OBJECTS := $(addsuffix .o,$(basename $(SOURCES)))
COMPONENT_OBJECTS := $(addsuffix .o,$(basename $(notdir $(COMPONENTS))))

.PHONY: all clean depend

all: image.boot

-include Makefile.depend

image.boot: depend _components.h _link.ld $(OBJECTS) $(COMPONENT_OBJECTS)
	$(LD) -no-check-sections -N -T _link.ld $(COMPONENT_OBJECTS) $(OBJECTS) -o $@ 

depend:
	-makedepend $(DEFS) $(CFLAGS) -f - $(SOURCES) > Makefile.depend 2> /dev/null

clean:
	-rm -f _components.h _link.ld $(COMPONENT_OBJECTS) $(OBJECTS) image.boot Makefile.depend 

_components.h _link.ld $(COMPONENT_OBJECTS): $(COMPONENTS)
	./pack $(IMAGE) $(OBJCOPY) $(COMPONENTS)

%.o: %.S
	$(CC) $(DEFS) $(CFLAGS) -D__ASM__ -c $< -o $@

%.o: %.c
	$(CC) $(DEFS) $(CFLAGS) -c $< -o $@
