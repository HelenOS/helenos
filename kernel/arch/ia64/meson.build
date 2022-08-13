#
# SPDX-FileCopyrightText: 2005 Martin Decky
# SPDX-FileCopyrightText: 2019 Jiří Zárevúcky
#
# SPDX-License-Identifier: BSD-3-Clause
#

arch_src += files(
	'src/start.S',
	'src/asm.S',
	'src/dummy.S',
	'src/ia64.c',
	'src/fpu_context.c',
	'src/context.S',
	'src/cpu/cpu.c',
	'src/debug/stacktrace.c',
	'src/debug/stacktrace_asm.S',
	'src/ivt.S',
	'src/interrupt.c',
	'src/mm/km.c',
	'src/mm/as.c',
	'src/mm/frame.c',
	'src/mm/page.c',
	'src/mm/tlb.c',
	'src/mm/vhpt.c',
	'src/proc/scheduler.c',
	'src/ddi/ddi.c',
	'src/smc.c',
	'src/smp/smp.c',
	'src/drivers/it.c',
)

if MACHINE == 'ski'
	arch_src += files('src/drivers/ski.c')
endif


_check_headers = [
	'include/arch/istate_struct.h',
	'include/arch/context_struct.h',
]

foreach h : _check_headers
	arch_src += [ autocheck.process(h) ]
endforeach
