#
# Copyright (c) 2011 Petr Koupy
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

USPACE_PREFIX = ../..
LIBRARY = libposix

EXTRA_CFLAGS = -Iinclude/

INCLUDE_LIBC = ./include/libc

LIBC_FILE = $(LIBC_PREFIX)/libc.a

FIXED_C_LIBRARY = libc4posix.a
MERGED_C_LIBRARY = libc.a
MERGE_LIBRARIES = \
	$(FIXED_C_LIBRARY) \
	$(LIBRARY).a \
	$(LIBSOFTFLOAT_PREFIX)/libsoftfloat.a \
	$(LIBSOFTINT_PREFIX)/libsoftint.a

REDEFS_HIDE_LIBC = redefs-hide-libc-symbols.list

PRE_DEPEND = $(INCLUDE_LIBC)
EXTRA_CLEAN = \
	$(INCLUDE_LIBC) \
	$(REDEFS_HIDE_LIBC) \
	libc.o

EXTRA_OUTPUT = $(FIXED_C_LIBRARY) $(FIXED_POSIX_LIBRARY) $(MERGED_C_LIBRARY)

SOURCES = \
	src/ctype.c \
	src/dlfcn.c \
	src/fcntl.c \
	src/fnmatch.c \
	src/locale.c \
	src/pthread/condvar.c \
	src/pthread/keys.c \
	src/pthread/mutex.c \
	src/pthread/threads.c \
	src/pwd.c \
	src/signal.c \
	src/stdio.c \
	src/stdio/scanf.c \
	src/stdlib.c \
	src/stdlib/strtold.c \
	src/string.c \
	src/strings.c \
	src/sys/mman.c \
	src/sys/stat.c \
	src/sys/wait.c \
	src/time.c \
	src/unistd.c

TEST_SOURCES = \
	test/main.c \
	test/scanf.c

include $(USPACE_PREFIX)/Makefile.common

$(INCLUDE_LIBC): ../c/include
	ln -s -f -n ../$^ $@

$(FIXED_C_LIBRARY): $(LIBC_FILE) $(REDEFS_HIDE_LIBC)
	$(OBJCOPY) --redefine-syms=$(REDEFS_HIDE_LIBC) $(LIBC_FILE) $@

$(MERGED_C_LIBRARY): $(MERGE_LIBRARIES)
	$(LD) --fatal-warnings -r -o libc.o --whole-archive $(MERGE_LIBRARIES) --no-whole-archive
	$(AR) rc $@ libc.o

$(REDEFS_HIDE_LIBC): $(SOURCES)
	sed -n -e 's/_HIDE_LIBC_SYMBOL(\(.*\))/\1 __helenos_libc_\1/p' $(SOURCES) >$@

