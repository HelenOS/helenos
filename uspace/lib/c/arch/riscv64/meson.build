#
# SPDX-FileCopyrightText: 2016 Martin Decky
#
# SPDX-License-Identifier: BSD-3-Clause
#

arch_src += [ autocheck.process('include/libarch/fibril_context.h') ]

arch_src += files(
	'src/entryjmp.c',
	'src/thread_entry.c',
	'src/fibril.c',
	'src/tls.c',
	'src/syscall.c',
	'src/stacktrace.c',
)

arch_start_src = files('src/crt0.c')
