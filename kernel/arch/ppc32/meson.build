#
# SPDX-FileCopyrightText: 2005 Martin Decky
#
# SPDX-License-Identifier: BSD-3-Clause
#

arch_src += files(
	'src/context.S',
	'src/debug/stacktrace.c',
	'src/debug/stacktrace_asm.S',
	'src/fpu_context.S',
	'src/boot/boot.S',
	'src/ppc32.c',
	'src/dummy.S',
	'src/exception.S',
	'src/interrupt.c',
	'src/asm.S',
	'src/cpu/cpu.c',
	'src/proc/scheduler.c',
	'src/smc.c',
	'src/mm/km.c',
	'src/mm/as.c',
	'src/mm/frame.c',
	'src/mm/page.c',
	'src/mm/pht.c',
	'src/mm/tlb.c',
	'src/drivers/pic.c',
)


_check_headers = [
	'include/arch/istate_struct.h',
	'include/arch/context_struct.h',
	'include/arch/fpu_context_struct.h',
]

foreach h : _check_headers
	arch_src += [ autocheck.process(h) ]
endforeach
