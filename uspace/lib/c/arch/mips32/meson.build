#
# SPDX-FileCopyrightText: 2005 Martin Decky
#
# SPDX-License-Identifier: BSD-3-Clause
#

arch_src += [ autocheck.process('include/libarch/fibril_context.h') ]

arch_src += files(
	'src/entryjmp.S',
	'src/thread_entry.S',
	'src/syscall.c',
	'src/fibril.S',
	'src/tls.c',
	'src/stacktrace.c',
	'src/stacktrace_asm.S',
)

arch_start_src = files('src/crt0.S')
