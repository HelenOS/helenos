#! /bin/bash

#
# Copyright (c) 2011 Petr Koupy
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

#
# This shell script is reserved for intrusive patches (hacks) to binutils
# that cannot be done in a clean and isolated way or for which doing so 
# would require too much complexity.
#
# List of patch descriptions:
#
# Patch 1
# Even though binutils build process supports cross compilation where
# build and host platforms are different, it is not easily applicable
# to HelenOS. It would be difficult to satisfy binutils expectations
# of host headers, libraries and tools on a build system (at least
# until these are developed/ported). Another issue would be the 
# necessity to carry out time consuming full canadian cross compilation
# (even in case when build, host and target hardware platforms are the
# same). Instead of going into such trouble, it is easier to leverage
# already  built HelenOS toolchain as a first stage of canadian cross
# and trick binutils scripts to do a simple cross compilation while 
# actually doing second stage of canadian cross. Because binutils
# configure scripts try to compile and execute various testing code, it
# have to be ensured that these tests are skipped. Such behaviour can
# be acomplished by patching cross compilation flag while leaving host
# and build parameters empty (i.e. configure script believes it is
# not doing cross compilation while skipping some testing as in the case
# of cross compilation).
# 
# Patch 2
# Enabled cross compilation flag brings along some anomalies which
# have to be reverted. 
#
# Patch 3
# Binutils plugin support is dependent on libdl.so library.
# By default, the plugin support is switched off for all
# configure scripts of binutils. The only exception is configure
# script of ld 2.21 (and possibly above), where plugin support
# became mandatory (although not really needed).
#
# Patch 4
# Whereas most systems maps pid_t to signed int, HelenOS maps it
# to 64-bit unsigned integer (which is further masked by libposix to
# 64-bit signed integer). Because libiberty blindly expects pid_t to be
# signed int, there is some type incompatibility in libiberty
# files related to executing a subprocess. Since both as and ld are
# not runtime dependent on this functionality, the simplest solution is
# to patch libiberty to avoid compiler warnings.
#
# Patch 5
# When host and target system is equal, libbfd wants to compile a support
# for core files, which is dependent on non-standard headers sys/param.h
# and sys/user.h. Since we are cross compiling even when host and target
# are equal, variables related to core file support must be cleared.
#
# Patch 6
# There is a few occurences in binutils where POSIX function is declared 
# and called without first including the corresponding header. Such
# declarations cause a problem to the linker, because all functions
# from libposix are prefixed with the posix_ prefix.
#
# Patch 7
# Implementation of fnmatch inside libiberty is not very friendly to the
# non-GNU libc which implements its own native fnmatch. To resolve this 
# incompatibility, libiberty fnmatch has to be manually hidden.
#
# Patch 8
# When building binutils for arm32 target, there is a conflict with
# libposix function name redefinitons in one of the arm-specific files.
#

case "$1" in
	"do")
		# Backup original files.
		cp -f "$2/configure" "$2/configure.backup"
		cp -f "$2/bfd/configure" "$2/bfd/configure.backup"
		cp -f "$2/gas/configure" "$2/gas/configure.backup"
		cp -f "$2/gas/config/tc-arm.c" "$2/gas/config/tc-arm.c.backup"
		cp -f "$2/intl/configure" "$2/intl/configure.backup"
		cp -f "$2/ld/configure" "$2/ld/configure.backup"
		cp -f "$2/libiberty/configure" "$2/libiberty/configure.backup"
		cp -f "$2/libiberty/Makefile.in" "$2/libiberty/Makefile.in.backup"
		cp -f "$2/include/fnmatch.h" "$2/include/fnmatch.h.backup"
		cp -f "$2/libiberty/fnmatch.c" "$2/libiberty/fnmatch.c.backup"
		cp -f "$2/libiberty/pex-common.h" "$2/libiberty/pex-common.h.backup"
		cp -f "$2/libiberty/xstrerror.c" "$2/libiberty/xstrerror.c.backup"
		cp -f "$2/opcodes/configure" "$2/opcodes/configure.backup"

		# Patch main binutils configure script.
		cat "$2/configure.backup" | \
		# See Patch 1.
		sed 's/^cross_compiling=no/cross_compiling=yes/g' \
		> "$2/configure"

		# Patch bfd configure script.
		cat "$2/bfd/configure.backup" | \
		# See Patch 1.
		sed 's/^cross_compiling=no/cross_compiling=yes/g' | \
		# See Patch 5.
		sed 's/COREFILE=".*"/COREFILE='\'\''/g' | \
		sed 's/COREFILE=[^ ]*/COREFILE='\'\''/g' \
		> "$2/bfd/configure"

		# Patch gas configure script.
		cat "$2/gas/configure.backup" | \
		# See Patch 1.
		sed 's/^cross_compiling=no/cross_compiling=yes/g' \
		> "$2/gas/configure"

		# Patch gas tc-arm.c.
		cat "$2/gas/config/tc-arm.c.backup" | \
		# See Patch 8.
		sed 's/\(#include "dwarf2dbg.h"\)/\1\n#undef div/g' \
		> "$2/gas/config/tc-arm.c"

		# Patch intl configure script.
		cat "$2/intl/configure.backup" | \
		# See Patch 1.
		sed 's/^cross_compiling=no/cross_compiling=yes/g' \
		> "$2/intl/configure"

		# Patch ld configure script.
		cat "$2/ld/configure.backup" | \
		# See Patch 1.
		sed 's/^cross_compiling=no/cross_compiling=yes/g' | \
		# See Patch 3.
		sed 's/^enable_plugins=yes/enable_plugins=no/g' \
		> "$2/ld/configure"

		# Patch libiberty configure script.
		cat "$2/libiberty/configure.backup" | \
		# See Patch 1.
		sed 's/^cross_compiling=no/cross_compiling=yes/g' \
		> "$2/libiberty/configure"

		# Hide libiberty fnmatch implementation.
		# See Patch 7.
		mv -f "$2/include/fnmatch.h" "$2/include/fnmatch_hide.h"
		(
		# Avoid compiler warning for empty compilation unit.
		echo 'char __fnmatch_hide(void);'
		echo 'char __fnmatch_hide(void) { return 0; }'
		echo '#define __GNU_LIBRARY__'
		cat "$2/libiberty/fnmatch.c.backup"
		) > "$2/libiberty/fnmatch.c"
		mv -f "$2/libiberty/fnmatch.c" "$2/libiberty/fnmatch_hide.c"
		cat "$2/libiberty/Makefile.in.backup" | \
		sed 's/fnmatch/fnmatch_hide/g' \
		> "$2/libiberty/Makefile.in"

		# Patch libiberty pex-common.h.
		cat "$2/libiberty/pex-common.h.backup" | \
		# See Patch 4.
		sed 's/pid_t (\*wait)/int (*wait)/g' \
		> "$2/libiberty/pex-common.h"

		# Patch libiberty xstrerror.c.
		# See Patch 6.
		(
		echo '#include <string.h>'
		echo '#define DONT_DECLARE_STRERROR'
		cat "$2/libiberty/xstrerror.c.backup"
		) > "$2/libiberty/xstrerror.c"

		# Patch opcodes configure script.
		cat "$2/opcodes/configure.backup" | \
		# See Patch 1.
		sed 's/^cross_compiling=no/cross_compiling=yes/g' | \
		# See Patch 2.
		sed 's/BUILD_LIBS=-liberty/BUILD_LIBS=..\/libiberty\/libiberty.a/g' \
		> "$2/opcodes/configure"

		;;
	"undo")
		# Restore original files.
		mv -f "$2/configure.backup" "$2/configure"
		mv -f "$2/bfd/configure.backup" "$2/bfd/configure"
		mv -f "$2/gas/configure.backup" "$2/gas/configure"
		mv -f "$2/gas/config/tc-arm.c.backup" "$2/gas/config/tc-arm.c"
		mv -f "$2/intl/configure.backup" "$2/intl/configure"
		mv -f "$2/ld/configure.backup" "$2/ld/configure"
		mv -f "$2/libiberty/configure.backup" "$2/libiberty/configure"
		mv -f "$2/libiberty/Makefile.in.backup" "$2/libiberty/Makefile.in"
		rm -f "$2/include/fnmatch_hide.h" "$2/libiberty/fnmatch_hide.c"
		mv -f "$2/include/fnmatch.h.backup" "$2/include/fnmatch.h"
		mv -f "$2/libiberty/fnmatch.c.backup" "$2/libiberty/fnmatch.c"
		mv -f "$2/libiberty/pex-common.h.backup" "$2/libiberty/pex-common.h"
		mv -f "$2/libiberty/xstrerror.c.backup" "$2/libiberty/xstrerror.c"
		mv -f "$2/opcodes/configure.backup" "$2/opcodes/configure"
		;;
	*)
		;;
esac

