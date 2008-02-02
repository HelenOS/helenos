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
include ../../Makefile.config

## Setup toolchain
#

LIBC_PREFIX = ../../lib/libc
SOFTINT_PREFIX = ../../lib/softint
include $(LIBC_PREFIX)/Makefile.toolchain

CFLAGS += \
	-DSHELL \
	-I. \
	-DNO_HISTORY \
	-DBSD=1 \
	-DSMALL \
	-D_GNU_SOURCE \
	-DGLOB_BROKEN \
	-D__COPYRIGHT\(x\)= \
	-D__RCSID\(x\)= 
#	-D_DIAGASSERT\(x\)= \
#	-DHETIO

LIBS = $(LIBC_PREFIX)/libc.a
DEFS += -DRELEASE=\"$(RELEASE)\"

ifdef REVISION
	DEFS += "-DREVISION=\"$(REVISION)\""
endif

ifdef TIMESTAMP
	DEFS += "-DTIMESTAMP=\"$(TIMESTAMP)\""
endif

## Sources
#

OUTPUT =	sh
SHSRCS =	\
	fake.c \
	alias.c \
	cd.c \
	bltin/echo.c \
	error.c \
	eval.c \
	exec.c \
	expand.c \
	hetio.c \
	histedit.c \
	input.c \
	jobs.c \
	mail.c \
	main.c \
	memalloc.c \
	miscbltin.c \
	mystring.c \
	options.c \
	output.c \
	parser.c \
	redir.c \
	show.c \
	setmode.c \
	bltin/test.c \
	bltin/times.c \
	trap.c \
	var.c 

GENSRCS = \
	builtins.c \
	init.c \
	nodes.c \
	syntax.c \
	signames.c 
#	lex.yy.c \
#	arith.c \
#	arith.h \

GENHEADERS = \
	token.h \
	nodes.h \
	syntax.h \
	builtins.h 

SOURCES =	${SHSRCS} ${GENSRCS}

OBJECTS := $(addsuffix .o,$(basename $(SOURCES)))

.PHONY: all clean depend disasm

all: $(OUTPUT) disasm

-include Makefile.depend

BUILD_TOOLS = \
	mkinit \
	mknodes \
	mksyntax

build-tools: $(BUILD_TOOLS)
	$(CC) -c $< -o $@

.ORDER: builtins.c builtins.h
builtins.c builtins.h: mkbuiltins builtins.def
	sh mkbuiltins shell.h builtins.def `pwd`

INIT_DEPS = alias.c eval.c exec.c input.c jobs.c options.c parser.c \
	redir.c trap.c var.c output.c
	
init.c: mkinit $(INIT_DEPS)
	./mkinit $(INIT_DEPS)

signames.c: mksignames
	./mksignames

nodes.c nodes.h: mknodes nodetypes nodes.c.pat
	./mknodes ./nodetypes ./nodes.c.pat

syntax.c syntax.h: mksyntax
	./mksyntax

#arith.c arith.h: arith.y
#	yacc -d arith.y
#	mv y.tab.h arith.h
#	mv y.tab.c arith.c

token.h: mktokens
	sh ./mktokens

clean:
	-rm -f $(OUTPUT) $(OUTPUT).map $(OUTPUT).disasm Makefile.depend
	-rm -f $(GENSRCS) $(GENHEADERS) $(OBJECTS) $(BUILD_TOOLS)

depend:
	$(CC) $(DEFS) $(CFLAGS) -M $(SOURCES) $(GENHEADERS) > Makefile.depend

$(OUTPUT): $(GENSRCS) $(GENHEADERS) $(OBJECTS) $(LIBS)
	$(LD) -T $(LIBC_PREFIX)/arch/$(ARCH)/_link.ld $(OBJECTS) $(LIBS) $(LFLAGS) -o $@ -Map $(OUTPUT).map

disasm:
	$(OBJDUMP) -d $(OUTPUT) >$(OUTPUT).disasm

%.o: %.S
	$(CC) $(DEFS) $(AFLAGS) $(CFLAGS) -D__ASM__ -c $< -o $@

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o: %.c
	$(CC) $(DEFS) $(CFLAGS) -c $< -o $@
