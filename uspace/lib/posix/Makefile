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
SLIBRARY = libposix.so.0.0
LSONAME = libposix.so0

EXTRA_CFLAGS = -Iinclude/

INCLUDE_LIBC = ./include/libc

LIBC_FILE = $(LIBC_PREFIX)/libc.a

FIXED_POSIX_LIBRARY = libposixaslibc.a
FIXED_C_LIBRARY = libc4posix.a

REDEFS_HIDE_LIBC = redefs-hide-libc-symbols.list
REDEFS_SHOW_LIBPOSIX = redefs-show-posix-symbols.list
COLLISIONS_LIST = collisions.list

PRE_DEPEND = $(INCLUDE_LIBC)
EXTRA_CLEAN = \
	$(INCLUDE_LIBC) \
	$(REDEFS_HIDE_LIBC) $(REDEFS_SHOW_LIBPOSIX) \
	$(COLLISIONS_LIST)
EXTRA_OUTPUT = $(FIXED_C_LIBRARY) $(FIXED_POSIX_LIBRARY)

SOURCES = \
	source/ctype.c \
	source/errno.c \
	source/fcntl.c \
	source/fnmatch.c \
	source/getopt.c \
	source/locale.c \
	source/pthread/condvar.c \
	source/pthread/keys.c \
	source/pthread/mutex.c \
	source/pthread/threads.c \
	source/pwd.c \
	source/signal.c \
	source/stdio.c \
	source/stdio/scanf.c \
	source/stdlib.c \
	source/stdlib/strtol.c \
	source/stdlib/strtold.c \
	source/string.c \
	source/strings.c \
	source/sys/mman.c \
	source/sys/stat.c \
	source/sys/wait.c \
	source/time.c \
	source/unistd.c

TEST_SOURCES = \
	test/main.c \
	test/scanf.c

include $(USPACE_PREFIX)/Makefile.common

$(INCLUDE_LIBC): ../c/include
	ln -s -f -n ../$^ $@

$(FIXED_C_LIBRARY): $(REDEFS_HIDE_LIBC) $(REDEFS_SHOW_LIBPOSIX)
	$(OBJCOPY) --redefine-syms=$(REDEFS_HIDE_LIBC) $(LIBC_FILE) $@
	$(OBJCOPY) --redefine-syms=$(REDEFS_SHOW_LIBPOSIX) $@ $@

$(FIXED_POSIX_LIBRARY): $(LIBRARY).a $(REDEFS_HIDE_LIBC) $(REDEFS_SHOW_LIBPOSIX)
	$(OBJCOPY) --redefine-syms=$(REDEFS_HIDE_LIBC) $(LIBRARY).a $@
	$(OBJCOPY) --redefine-syms=$(REDEFS_SHOW_LIBPOSIX) $@ $@

$(REDEFS_HIDE_LIBC): $(COLLISIONS_LIST)
	sed 's/.*/& __helenos_libc_&/' <$(COLLISIONS_LIST) >$@

$(REDEFS_SHOW_LIBPOSIX): $(COLLISIONS_LIST)
	sed 's/.*/posix_& &/' <$(COLLISIONS_LIST) >$@

$(COLLISIONS_LIST):
	find ./include/posix -name '*.h' -exec \
		sed -n -e '/^#/d' -e 's/__POSIX_DEF__/\n&/gp' {} \; | \
		sed -n -e 's/__POSIX_DEF__(\([^)]*\)).*/\1/p' | \
		sort -u >$@
