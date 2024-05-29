#
# Copyright (c) 2024 Jiri Svoboda
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

# libarch
arch_src = []
subdir('arch' / UARCH)

c_args = [ '-fno-builtin', '-D_LIBC_SOURCE' ]

root_path = '..' / '..' / '..'

incdirs = [
	'include',
	'arch' / UARCH / 'include',
	root_path / 'abi' / 'arch' / UARCH / 'include',
	root_path / 'abi' / 'include',
	root_path / 'common' / 'include',
]

includes += include_directories(incdirs, is_system: true)

allow_shared = true

uspace_lib_devel_install_script_text += 'mkdir -p "${DESTDIR}include/libc"'
foreach idir : incdirs
	_sdir = meson.current_source_dir() / idir
	uspace_lib_devel_install_script_text += 'cp -R -L -T "@0@" "${DESTDIR}include/libc"'.format(_sdir)
	exported_devel_files += ['include', _sdir, 'libc']
endforeach

src = [ arch_src ]

src += files(
	'common/adt/checksum.c',
	'common/adt/circ_buf.c',
	'common/adt/list.c',
	'common/adt/hash_table.c',
	'common/adt/odict.c',
	'common/printf/printf_core.c',
	'common/stdc/ctype.c',
	'common/stdc/mem.c',
	'common/stdc/bsearch.c',
	'common/stdc/qsort.c',
	'common/stdc/calloc.c',
	'common/gsort.c',
	'common/str.c',
	'common/str_error.c',
	'common/strtol.c',

	'generic/libc.c',
	'generic/adt/prodcons.c',
	'generic/as.c',
	'generic/ddi.c',
	'generic/perm.c',
	'generic/capa.c',
	'generic/config.c',
	'generic/context.c',
	'generic/dbgcon.c',
	'generic/device/clock_dev.c',
	'generic/device/hw_res.c',
	'generic/device/hw_res_parsed.c',
	'generic/device/pio_window.c',
	'generic/dirent.c',
	'generic/dlfcn.c',
	'generic/elf/elf.c',
	'generic/elf/elf_load.c',
	'generic/elf/elf_mod.c',
	'generic/event.c',
	'generic/errno.c',
	'generic/inttypes.c',
	'generic/loc.c',
	'generic/string.c',
	'generic/l18n/langs.c',
	'generic/pcb.c',
	'generic/pio_trace.c',
	'generic/smc.c',
	'generic/task.c',
	'generic/imath.c',
	'generic/io/asprintf.c',
	'generic/io/io.c',
	'generic/io/printf.c',
	'generic/io/log.c',
	'generic/io/logctl.c',
	'generic/io/kio.c',
	'generic/io/klog.c',
	'generic/io/snprintf.c',
	'generic/io/vprintf.c',
	'generic/io/vsnprintf.c',
	'generic/io/table.c',
	'generic/irq.c',
	'generic/ieee_double.c',
	'generic/power_of_ten.c',
	'generic/double_to_str.c',
	'generic/malloc.c',
	'generic/rndgen.c',
	'generic/stdio/scanf.c',
	'generic/stdio/sprintf.c',
	'generic/stdio/sscanf.c',
	'generic/stdio/sstream.c',
	'generic/stdio/vsprintf.c',
	'generic/thread/fibril.c',
	'generic/thread/fibril_synch.c',
	'generic/thread/thread.c',
	'generic/thread/tls.c',
	'generic/thread/futex.c',
	'generic/thread/mpsc.c',
	'generic/sysinfo.c',
	'generic/ipc.c',
	'generic/ns.c',
	'generic/async/client.c',
	'generic/async/server.c',
	'generic/async/ports.c',
	'generic/loader.c',
	'generic/getopt.c',
	'generic/time.c',
	'generic/tmpfile.c',
	'generic/stdio.c',
	'generic/stdlib.c',
	'generic/udebug.c',
	'generic/vfs/canonify.c',
	'generic/vfs/inbox.c',
	'generic/vfs/mtab.c',
	'generic/vfs/vfs.c',
	'generic/setjmp.c',
	'generic/stack.c',
	'generic/stacktrace.c',
	'generic/arg_parse.c',
	'generic/stats.c',
	'generic/assert.c',
	'generic/ubsan.c',
	'generic/uuid.c',
)

if CONFIG_RTLD
	src += files(
		'generic/rtld/rtld.c',
		'generic/rtld/dynamic.c',
		'generic/rtld/module.c',
		'generic/rtld/symbol.c',
	)
endif

test_src = files(
	'test/adt/circ_buf.c',
	'test/adt/odict.c',
	'test/capa.c',
	'test/casting.c',
	'test/double_to_str.c',
	'test/fibril/timer.c',
	'test/getopt.c',
	'test/gsort.c',
	'test/ieee_double.c',
	'test/imath.c',
	'test/inttypes.c',
	'test/io/table.c',
	'test/loc.c',
	'test/main.c',
	'test/mem.c',
	'test/perf.c',
	'test/perm.c',
	'test/qsort.c',
	'test/sprintf.c',
	'test/stdio/scanf.c',
	'test/stdio.c',
	'test/stdlib.c',
	'test/str.c',
	'test/string.c',
	'test/strtol.c',
	'test/uuid.c',
)

# Startfiles.
# This is what other systems know as crt*.o files, i.e. a set of object files
# that are automatically built into every program binary (but not libraries).
# We haven't yet installed the object files where the compiler can find them,
# so we have to add them manually.
#

start_src = [ arch_start_src, files('generic/crt/crt1.c') ]

libstartfiles = static_library('startfiles', start_src,
	include_directories: includes,
	c_args: arch_uspace_c_args + c_args,
	pic: false,
)
exported_devel_files += ['staticlib', libstartfiles, 'libstartfiles.a']

if CONFIG_DEVEL_FILES
	install_files += [[ 'lib', libstartfiles.full_path(), 'libstartfiles.a' ]]
	install_deps += [ libstartfiles ]
endif
