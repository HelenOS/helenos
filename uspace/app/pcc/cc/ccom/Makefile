#
# Copyright (c) 2011 Jiri Zarevucky
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

USPACE_PREFIX = ../../../..
MIPDIR = ../../mip
MDIR = ../../arch/$(PLATFORM)
OSDIR = ../../os/helenos
EXTRA_CFLAGS = -I$(MIPDIR) -I$(MDIR) -I$(OSDIR) -I. -w
DEFS = -Dmach_$(PLATFORM) -D__helenos__ -DPCC_DEBUG -DGCC_COMPAT -Dfree=free
LIBS = $(LIBSOFTFLOAT_PREFIX)/libsoftfloat.a
BINARY = ccom

PRE_DEPEND = cgram.c scan.c external.c external.h common.c compat.c
EXTRA_CLEAN = cgram.c cgram.h scan.c external.c external.h common.c compat.c \
	$(MIPDIR)/*.o $(MDIR)/*.o

POSIX_COMPAT = y

# FIXME: external.{c,h} must be generated for each target system

SOURCES = \
	external.c \
	$(MDIR)/local.c \
	$(MDIR)/local2.c \
	$(MDIR)/code.c \
	$(MDIR)/order.c \
	$(MDIR)/table.c \
	$(MIPDIR)/match.c \
	$(MIPDIR)/reader.c \
	$(MIPDIR)/optim2.c \
	$(MIPDIR)/regs.c \
	common.c \
	compat.c \
	cgram.c \
	scan.c \
	builtins.c \
	gcc_compat.c \
	init.c \
	inline.c \
	main.c \
	optim.c \
	pftn.c \
	softfloat.c \
	stabs.c \
	symtabs.c \
	trees.c

include $(USPACE_PREFIX)/Makefile.common

cgram.c: cgram.y
	$(YACC) $(YFLAGS) -d $<
	mv y.tab.c cgram.c
	mv y.tab.h cgram.h

scan.c: scan.l cgram.c
	$(LEX) $(LFLAGS) $<
	mv lex.yy.c scan.c

compat.c: $(MIPDIR)/compat.c
	ln -s -f $^ $@

common.c: $(MIPDIR)/common.c
	ln -s -f $^ $@

external.c: arch/$(PLATFORM)/external.c
	ln -s -f $^ $@

external.h: arch/$(PLATFORM)/external.h
	ln -s -f $^ $@

