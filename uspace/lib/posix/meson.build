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

includes += include_directories('include/posix', 'include')
c_args += [ '-fno-builtin', '-D_XOPEN_SOURCE' ]

# TODO

src = files(
	'src/ctype.c',
	'src/dlfcn.c',
	'src/fcntl.c',
	'src/fnmatch.c',
	'src/locale.c',
	'src/pthread/condvar.c',
	'src/pthread/keys.c',
	'src/pthread/mutex.c',
	'src/pthread/threads.c',
	'src/pwd.c',
	'src/signal.c',
	'src/stdio.c',
	'src/stdlib.c',
	'src/string.c',
	'src/strings.c',
	'src/sys/mman.c',
	'src/sys/stat.c',
	'src/sys/wait.c',
	'src/time.c',
	'src/unistd.c',
)

test_src = files(
	'test/main.c',
	'test/stdio.c',
	'test/stdlib.c',
	'test/unistd.c',
)

_sdir = meson.current_source_dir() / 'include' / 'posix'
uspace_lib_devel_install_script_text += 'mkdir -p "${DESTDIR}include/libposix"'
uspace_lib_devel_install_script_text += 'cp -R -L -T "@0@" "${DESTDIR}include/libposix"'.format(_sdir)
uspace_lib_devel_install_script_text += 'ln -s -r "${DESTDIR}include/libc" "${DESTDIR}/include/common"'

exported_devel_files += [ 'include', meson.current_source_dir() / 'include' / 'posix', 'libposix' ]
exported_devel_files += [ 'includesymlink', 'libc', 'libposix' ]
