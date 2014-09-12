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

CONFIG_MAKEFILE = $(ROOT_PATH)/Makefile.config
COMMON_HEADER_ARCH = arch/$(UARCH)/include/libarch/common.h

LINKER_SCRIPTS = \
	$(LIBC_PREFIX)/arch/$(UARCH)/_link.ld \
	$(LIBC_PREFIX)/arch/$(UARCH)/_link-loader.ld \
	$(LIBC_PREFIX)/arch/$(UARCH)/_link-shlib.ld \
	$(LIBC_PREFIX)/arch/$(UARCH)/_link-dlexe.ld

PRE_DEPEND = $(COMMON_HEADER_ARCH)
EXTRA_OUTPUT = $(LINKER_SCRIPTS)
EXTRA_CLEAN = $(COMMON_HEADER_ARCH) $(LINKER_SCRIPTS)
LIBRARY = libc
SLIBRARY = libc.so.0.0
LSONAME = libc.so0

-include $(CONFIG_MAKEFILE)
-include arch/$(UARCH)/Makefile.inc

ARCH_AUTOGENS_H := $(addsuffix .h,$(basename $(ARCH_AUTOGENS_AG)))
ARCH_AUTOGENS_PROBE_C := $(addsuffix .ag.probe.c,$(basename $(ARCH_AUTOGENS_AG)))
ARCH_AUTOGENS_PROBE_S := $(addsuffix .ag.probe.s,$(basename $(ARCH_AUTOGENS_AG)))

PRE_DEPEND += $(ARCH_AUTOGENS_H)
EXTRA_CLEAN += $(ARCH_AUTOGENS_H) $(ARCH_AUTOGENS_PROBE_C) $(ARCH_AUTOGENS_PROBE_S)

GENERIC_SOURCES = \
	generic/libc.c \
	generic/ddi.c \
	generic/as.c \
	generic/bd.c \
	generic/bd_srv.c \
	generic/cap.c \
	generic/cfg.c \
	generic/clipboard.c \
	generic/corecfg.c \
	generic/devman.c \
	generic/device/hw_res.c \
	generic/device/hw_res_parsed.c \
	generic/device/pio_window.c \
	generic/device/clock_dev.c \
	generic/device/led_dev.c \
	generic/dhcp.c \
	generic/dnsr.c \
	generic/dlfcn.c \
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
	generic/inet/addr.c \
	generic/inet.c \
	generic/inetcfg.c \
	generic/inetping.c \
	generic/io/asprintf.c \
	generic/io/input.c \
	generic/io/io.c \
	generic/io/chardev.c \
	generic/io/chardev_srv.c \
	generic/io/chargrid.c \
	generic/io/output.c \
	generic/io/printf.c \
	generic/io/log.c \
	generic/io/logctl.c \
	generic/io/kio.c \
	generic/io/klog.c \
	generic/io/snprintf.c \
	generic/io/vprintf.c \
	generic/io/vsnprintf.c \
	generic/io/printf_core.c \
	generic/io/con_srv.c \
	generic/io/console.c \
	generic/io/visualizer.c \
	generic/io/window.c \
	generic/iplink.c \
	generic/iplink_srv.c \
	generic/irc.c \
	generic/irq.c \
	generic/ieee_double.c \
	generic/power_of_ten.c \
	generic/double_to_str.c \
	generic/malloc.c \
	generic/sysinfo.c \
	generic/ipc.c \
	generic/ns.c \
	generic/async.c \
	generic/loader.c \
	generic/getopt.c \
	generic/adt/checksum.c \
	generic/adt/list.c \
	generic/adt/hash_table.c \
	generic/adt/dynamic_fifo.c \
	generic/adt/char_map.c \
	generic/adt/prodcons.c \
	generic/time.c \
	generic/stdlib.c \
	generic/udebug.c \
	generic/vfs/vfs.c \
	generic/vfs/canonify.c \
	generic/net/inet.c \
	generic/net/socket_client.c \
	generic/net/socket_parse.c \
	generic/setjmp.c \
	generic/stack.c \
	generic/stacktrace.c \
	generic/arg_parse.c \
	generic/sort.c \
	generic/stats.c \
	generic/assert.c \
	generic/pio_trace.c

ifeq ($(CONFIG_RTLD),y)
	GENERIC_SOURCES += \
		generic/rtld/rtld.c \
		generic/rtld/dynamic.c \
		generic/rtld/module.c \
		generic/rtld/symbol.c
endif

SOURCES = \
	$(GENERIC_SOURCES) \
	$(ARCH_SOURCES)

TEST_SOURCES = \
	test/main.c \
	test/sprintf.c

include $(USPACE_PREFIX)/Makefile.common

%.h: %.ag
	$(AUTOGEN) probe $< >$<.probe.c
	$(CC) $(DEFS) $(CFLAGS) -S -o $<.probe.s $<.probe.c
	$(AUTOGEN) generate $< <$<.probe.s >$@   

$(LIBC_PREFIX)/arch/$(UARCH)/_link.ld: $(LIBC_PREFIX)/arch/$(UARCH)/_link.ld.in
	$(CC) $(DEFS) $(CFLAGS) -DLIBC_PATH=$(CURDIR) -E -x c $< | grep -v "^\#" > $@

$(LIBC_PREFIX)/arch/$(UARCH)/_link-loader.ld: $(LIBC_PREFIX)/arch/$(UARCH)/_link.ld.in
	$(CC) $(DEFS) $(CFLAGS) -DLIBC_PATH=$(CURDIR) -DLOADER -E -x c $< | grep -v "^\#" > $@

$(LIBC_PREFIX)/arch/$(UARCH)/_link-shlib.ld: $(LIBC_PREFIX)/arch/$(UARCH)/_link.ld.in
	$(CC) $(DEFS) $(CFLAGS) -DLIBC_PATH=$(CURDIR) -DSHLIB -E -x c $< | grep -v "^\#" > $@

$(LIBC_PREFIX)/arch/$(UARCH)/_link-dlexe.ld: $(LIBC_PREFIX)/arch/$(UARCH)/_link.ld.in
	$(CC) $(DEFS) $(CFLAGS) -DLIBC_PATH=$(CURDIR) -DDLEXE -E -x c $< | grep -v "^\#" > $@

$(COMMON_HEADER_ARCH): $(COMMON_HEADER)
	ln -sfn ../../../../$< $@
