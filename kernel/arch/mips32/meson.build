#
# SPDX-FileCopyrightText: 2005 Martin Decky
#
# SPDX-License-Identifier: BSD-3-Clause
#

arch_src += files(
	'src/start.S',
	'src/context.S',
	'src/mips32.c',
	'src/asm.S',
	'src/exception.c',
	'src/interrupt.c',
	'src/cache.c',
	'src/debugger.c',
	'src/cpu/cpu.c',
	'src/debug/stacktrace.c',
	'src/debug/stacktrace_asm.S',
	'src/mm/km.c',
	'src/mm/frame.c',
	'src/mm/page.c',
	'src/mm/tlb.c',
	'src/mm/as.c',
	'src/fpu_context.c',
	'src/smc.c',
	'src/smp/smp.c',
	'src/machine_func.c',
)

if MACHINE == 'lmalta' or MACHINE == 'bmalta'
	arch_src += files('src/mach/malta/malta.c')

elif MACHINE == 'msim'
	arch_src += files(
		'src/mach/msim/msim.c',
		'src/mach/msim/dorder.c',
	)

endif


_check_headers = [
	'include/arch/istate_struct.h',
	'include/arch/context_struct.h',
	'include/arch/fpu_context_struct.h',
]

foreach h : _check_headers
	arch_src += [ autocheck.process(h) ]
endforeach
