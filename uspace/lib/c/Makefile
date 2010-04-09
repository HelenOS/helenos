#
# Copyright (c) 2005 Martin Decky
# Copyright (c) 2007 Jakub Jermar
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
ROOT_PATH = $(USPACE_PREFIX)/..

INCLUDE_KERNEL = include/kernel
INCLUDE_ARCH = include/arch
INCLUDE_LIBARCH = include/libarch

COMMON_MAKEFILE = $(ROOT_PATH)/Makefile.common
COMMON_HEADER = $(ROOT_PATH)/common.h
COMMON_HEADER_ARCH = arch/$(UARCH)/include/common.h

CONFIG_MAKEFILE = $(ROOT_PATH)/Makefile.config

PRE_DEPEND = $(INCLUDE_KERNEL) $(INCLUDE_ARCH) $(INCLUDE_LIBARCH) $(COMMON_HEADER_ARCH)
EXTRA_OUTPUT = $(LINKER_SCRIPT)
EXTRA_CLEAN = $(INCLUDE_KERNEL) $(INCLUDE_ARCH) $(INCLUDE_LIBARCH) $(COMMON_HEADER_ARCH) $(LINKER_SCRIPT)
LIBRARY = libc

-include $(COMMON_MAKEFILE)
-include $(CONFIG_MAKEFILE)
-include arch/$(UARCH)/Makefile.inc

GENERIC_SOURCES = \
	generic/libc.c \
	generic/ddi.c \
	generic/as.c \
	generic/cap.c \
	generic/clipboard.c \
	generic/devmap.c \
	generic/event.c \
	generic/errno.c \
	generic/mem.c \
	generic/str.c \
	generic/str_error.c \
	generic/fibril.c \
	generic/fibril_synch.c \
	generic/pcb.c \
	generic/smc.c \
	generic/thread.c \
	generic/tls.c \
	generic/task.c \
	generic/futex.c \
	generic/io/asprintf.c \
	generic/io/io.c \
	generic/io/printf.c \
	generic/io/klog.c \
	generic/io/snprintf.c \
	generic/io/vprintf.c \
	generic/io/vsnprintf.c \
	generic/io/printf_core.c \
	generic/io/console.c \
	generic/malloc.c \
	generic/sysinfo.c \
	generic/ipc.c \
	generic/async.c \
	generic/loader.c \
	generic/getopt.c \
	generic/adt/list.o \
	generic/adt/hash_table.o \
	generic/time.c \
	generic/err.c \
	generic/stdlib.c \
	generic/mman.c \
	generic/udebug.c \
	generic/vfs/vfs.c \
	generic/vfs/canonify.c \
	generic/stacktrace.c \
	generic/arg_parse.c

SOURCES = \
	$(GENERIC_SOURCES) \
	$(ARCH_SOURCES)

include $(USPACE_PREFIX)/Makefile.common

$(INCLUDE_ARCH): $(INCLUDE_KERNEL) $(INCLUDE_KERNEL)/arch
	ln -sfn kernel/arch $@

$(INCLUDE_LIBARCH): arch/$(UARCH)/include
	ln -sfn ../$< $@

$(INCLUDE_KERNEL)/arch: ../../../kernel/generic/include/arch $(INCLUDE_KERNEL)

$(INCLUDE_KERNEL): ../../../kernel/generic/include/
	ln -sfn ../$< $@

$(LINKER_SCRIPT): $(LINKER_SCRIPT).in
	$(GCC) $(DEFS) $(CFLAGS) -DLIBC_PATH=$(CURDIR) -E -x c $< | grep -v "^\#" > $@

$(COMMON_HEADER_ARCH): $(COMMON_HEADER)
	ln -sfn ../../../$< $@
