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

EXTRA_OUTPUT = $(START_FILES)
EXTRA_CFLAGS += -fno-builtin
LIBRARY = libc
SOVERSION = 0.0

-include $(CONFIG_MAKEFILE)
-include arch/$(UARCH)/Makefile.inc

GENERIC_SOURCES = \
	generic/libc.c \
	generic/ddi.c \
	generic/as.c \
	generic/bd.c \
	generic/bd_srv.c \
	generic/perm.c \
	generic/cap.c \
	generic/clipboard.c \
	generic/config.c \
	generic/context.c \
	generic/corecfg.c \
	generic/devman.c \
	generic/device/hw_res.c \
	generic/device/hw_res_parsed.c \
	generic/device/pio_window.c \
	generic/device/clock_dev.c \
	generic/device/led_dev.c \
	generic/dirent.c \
	generic/dhcp.c \
	generic/dnsr.c \
	generic/dlfcn.c \
	generic/elf/elf.c \
	generic/elf/elf_load.c \
	generic/elf/elf_mod.c \
	generic/event.c \
	generic/errno.c \
	generic/gsort.c \
	generic/inttypes.c \
	generic/loc.c \
	generic/mem.c \
	generic/str.c \
	generic/string.c \
	generic/str_error.c \
	generic/strtol.c \
	generic/l18n/langs.c \
	generic/pcb.c \
	generic/smc.c \
	generic/smp_memory_barrier.c \
	generic/task.c \
	generic/imath.c \
	generic/inet/addr.c \
	generic/inet/endpoint.c \
	generic/inet/host.c \
	generic/inet/hostname.c \
	generic/inet/hostport.c \
	generic/inet/tcp.c \
	generic/inet/udp.c \
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
	generic/io/label.c \
	generic/io/kio.c \
	generic/io/klog.c \
	generic/io/serial.c \
	generic/io/snprintf.c \
	generic/io/vprintf.c \
	generic/io/vsnprintf.c \
	generic/io/printf_core.c \
	generic/io/con_srv.c \
	generic/io/console.c \
	generic/io/table.c \
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
	generic/rndgen.c \
	generic/stdio/scanf.c \
	generic/stdio/sprintf.c \
	generic/stdio/sscanf.c \
	generic/stdio/sstream.c \
	generic/stdio/vsprintf.c \
	generic/thread/fibril.c \
	generic/thread/fibril_synch.c \
	generic/thread/thread.c \
	generic/thread/tls.c \
	generic/thread/futex.c \
	generic/thread/rcu.c \
	generic/thread/mpsc.c \
	generic/sysinfo.c \
	generic/ipc.c \
	generic/ns.c \
	generic/async/client.c \
	generic/async/server.c \
	generic/async/ports.c \
	generic/loader.c \
	generic/getopt.c \
	generic/adt/checksum.c \
	generic/adt/circ_buf.c \
	generic/adt/list.c \
	generic/adt/hash_table.c \
	generic/adt/odict.c \
	generic/adt/prodcons.c \
	generic/time.c \
	generic/tmpfile.c \
	generic/stdio.c \
	generic/stdlib.c \
	generic/udebug.c \
	generic/vfs/canonify.c \
	generic/vfs/inbox.c \
	generic/vfs/mtab.c \
	generic/vfs/vfs.c \
	generic/setjmp.c \
	generic/stack.c \
	generic/stacktrace.c \
	generic/arg_parse.c \
	generic/stats.c \
	generic/assert.c \
	generic/bsearch.c \
	generic/pio_trace.c \
	generic/qsort.c \
	generic/ubsan.c \
	generic/uuid.c \
	generic/vbd.c \
	generic/vol.c

ifeq ($(CONFIG_RTLD),y)
	GENERIC_SOURCES += \
		generic/rtld/rtld.c \
		generic/rtld/dynamic.c \
		generic/rtld/module.c \
		generic/rtld/symbol.c
endif

ARCH_SOURCES += $(ARCH_AUTOCHECK_HEADERS:%.h=%.check.c)

SOURCES = \
	$(GENERIC_SOURCES) \
	$(ARCH_SOURCES)

TEST_SOURCES = \
	test/adt/circ_buf.c \
	test/fibril/timer.c \
	test/main.c \
	test/mem.c \
	test/inttypes.c \
	test/io/table.c \
	test/stdio/scanf.c \
	test/odict.c \
	test/qsort.c \
	test/sprintf.c \
	test/stdio.c \
	test/stdlib.c \
	test/str.c \
	test/string.c

include $(USPACE_PREFIX)/Makefile.common

$(LIBC_PREFIX)/crt0.o: $(LIBC_PREFIX)/arch/$(UARCH)/src/entry.o
	cp $< $@

$(LIBC_PREFIX)/crt1.o: $(LIBC_PREFIX)/generic/crt/entry.o
	cp $< $@

AUTOCHECK = $(realpath $(ROOT_PATH)/tools/autocheck.awk)

%.check.c: %.h $(AUTOCHECK)
	cd $(<D) && $(AUTOCHECK) $(<F) > $(@F)
