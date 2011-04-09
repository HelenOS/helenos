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

# TODO: prefer libcompat headers to libc

ARCH = i386  # FIXME: figure out the real arch

USPACE_PREFIX = ../../../..
LIBS = $(LIBCOMPAT_PREFIX)/libcompat.a
MIPDIR = ../../mip
ARCHDIR = ../../arch/$(ARCH)
EXTRA_CFLAGS = -I$(LIBCOMPAT_PREFIX) -I$(MIPDIR) -I$(ARCHDIR)
EXTRA_CLEAN = y.tab.c y.tab.h
DEFS = -DCPP_DEBUG -Dfree=free -DCPPBUF=16384
BINARY = cpp

YACC = yacc

SOURCES = \
	y.tab.c \
	cpp.c \
	token.c \
	$(MIPDIR)/compat.c

y.tab.c:
	$(YACC) -d cpy.y

include $(USPACE_PREFIX)/Makefile.common

