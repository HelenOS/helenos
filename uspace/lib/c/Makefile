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

INCLUDE_ABI = include/abi
INCLUDE_LIBARCH = include/libarch

COMMON_MAKEFILE = $(ROOT_PATH)/Makefile.common
COMMON_HEADER = $(ROOT_PATH)/common.h
COMMON_HEADER_ARCH = arch/$(UARCH)/include/common.h

CONFIG_MAKEFILE = $(ROOT_PATH)/Makefile.config

LINKER_SCRIPTS = \
	$(LIBC_PREFIX)/arch/$(UARCH)/_link.ld \
	$(LIBC_PREFIX)/arch/$(UARCH)/_link-loader.ld \
	$(LIBC_PREFIX)/arch/$(UARCH)/_link-shlib.ld \
	$(LIBC_PREFIX)/arch/$(UARCH)/_link-dlexe.ld

PRE_DEPEND = $(INCLUDE_ABI) $(INCLUDE_LIBARCH) $(COMMON_HEADER_ARCH)
EXTRA_OUTPUT = $(LINKER_SCRIPTS)
EXTRA_CLEAN = $(INCLUDE_ABI) $(INCLUDE_LIBARCH) $(COMMON_HEADER_ARCH) $(LINKER_SCRIPTS)
LIBRARY = libc
SLIBRARY = libc.so.0.0
LSONAME = libc.so0


-include $(COMMON_MAKEFILE)
-include $(CONFIG_MAKEFILE)
-include arch/$(UARCH)/Makefile.inc

GENERIC_SOURCES = \
	generic/libc.c \
	generic/ddi.c \
	generic/as.c \
	generic/cap.c \
	generic/cfg.c \
	generic/clipboard.c \
	generic/devman.c \
	generic/device/hw_res.c \
	generic/device/hw_res_parsed.c \
	generic/device/char_dev.c \
	generic/device/nic.c \
	generic/device/pci.c \
	generic/device/ahci.c \
	generic/elf/elf_load.c \
	generic/event.c \
	generic/errno.c \
	generic/loc.c \
	generic/mem.c \
	generic/str.c \
	generic/str_error.c \
	generic/l18n/langs.c \
	generic/fibril.c \
	generic/fibril_synch.c \
	generic/pcb.c \
	generic/smc.c \
	generic/thread.c \
	generic/tls.c \
	generic/task.c \
	generic/futex.c \
	generic/inet.c \
	generic/inetcfg.c \
	generic/inetping.c \
	generic/io/asprintf.c \
	generic/io/io.c \
	generic/io/printf.c \
	generic/io/log.c \
	generic/io/klog.c \
	generic/io/snprintf.c \
	generic/io/vprintf.c \
	generic/io/vsnprintf.c \
	generic/io/printf_core.c \
	generic/io/console.c \
	generic/iplink.c \
	generic/iplink_srv.c \
	generic/malloc.c \
	generic/sysinfo.c \
	generic/ipc.c \
	generic/ns.c \
	generic/async.c \
	generic/loader.c \
	generic/getopt.c \
	generic/adt/list.c \
	generic/adt/hash_table.c \
	generic/adt/hash_set.c \
	generic/adt/dynamic_fifo.c \
	generic/adt/char_map.c \
	generic/adt/prodcons.c \
	generic/time.c \
	generic/stdlib.c \
	generic/mman.c \
	generic/udebug.c \
	generic/vfs/vfs.c \
	generic/vfs/canonify.c \
	generic/net/inet.c \
	generic/net/socket_client.c \
	generic/net/socket_parse.c \
	generic/stacktrace.c \
	generic/arg_parse.c \
	generic/sort.c \
	generic/stats.c \
	generic/assert.c

ifeq ($(CONFIG_RTLD),y)
	GENERIC_SOURCES += \
		generic/dlfcn.c \
		generic/rtld/rtld.c \
		generic/rtld/dynamic.c \
		generic/rtld/module.c \
		generic/rtld/symbol.c
endif

SOURCES = \
	$(GENERIC_SOURCES) \
	$(ARCH_SOURCES)

include $(USPACE_PREFIX)/Makefile.common

$(INCLUDE_LIBARCH): arch/$(UARCH)/include
	ln -sfn ../$< $@

$(INCLUDE_ABI): ../../../abi/include/
	ln -sfn ../$< $@

$(LIBC_PREFIX)/arch/$(UARCH)/_link.ld: $(LIBC_PREFIX)/arch/$(UARCH)/_link.ld.in
	$(GCC) $(DEFS) $(CFLAGS) -DLIBC_PATH=$(CURDIR) -E -x c $< | grep -v "^\#" > $@

$(LIBC_PREFIX)/arch/$(UARCH)/_link-loader.ld: $(LIBC_PREFIX)/arch/$(UARCH)/_link.ld.in
	$(GCC) $(DEFS) $(CFLAGS) -DLIBC_PATH=$(CURDIR) -DLOADER -E -x c $< | grep -v "^\#" > $@

$(LIBC_PREFIX)/arch/$(UARCH)/_link-shlib.ld: $(LIBC_PREFIX)/arch/$(UARCH)/_link.ld.in
	$(GCC) $(DEFS) $(CFLAGS) -DLIBC_PATH=$(CURDIR) -DSHLIB -E -x c $< | grep -v "^\#" > $@

$(LIBC_PREFIX)/arch/$(UARCH)/_link-dlexe.ld: $(LIBC_PREFIX)/arch/$(UARCH)/_link.ld.in
	$(GCC) $(DEFS) $(CFLAGS) -DLIBC_PATH=$(CURDIR) -DDLEXE -E -x c $< | grep -v "^\#" > $@

$(COMMON_HEADER_ARCH): $(COMMON_HEADER)
	ln -sfn ../../../$< $@
