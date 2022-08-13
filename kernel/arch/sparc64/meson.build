#
# SPDX-FileCopyrightText: 2005 Martin Decky
#
# SPDX-License-Identifier: BSD-3-Clause
#

if PROCESSOR == 'us' or PROCESSOR == 'us3'
	USARCH = 'sun4u'
elif PROCESSOR == 'sun4v'
	USARCH = 'sun4v'
else
	error('Unknown PROCESSOR: ' + PROCESSOR)
endif

arch_src += files(
	'src/sparc64.c',
	'src/cpu' / USARCH / 'cpu.c',
	'src/debug/stacktrace.c',
	'src/debug/stacktrace_asm.S',
	'src/asm.S',
	'src' / USARCH / 'asm.S',
	'src/console.c',
	'src/context.S',
	'src/fpu_context.c',
	'src/dummy.S',
	'src/mm' / USARCH / 'km.c',
	'src/mm' / USARCH / 'as.c',
	'src/mm' / USARCH / 'frame.c',
	'src/mm/page.c',
	'src/mm' / USARCH / 'tlb.c',
	'src' / USARCH / 'sparc64.c',
	'src' / USARCH / 'start.S',
	'src/proc' / USARCH / 'scheduler.c',
	'src/proc/thread.c',
	'src/smc.c',
	'src/trap' / USARCH / 'mmu.S',
	'src/trap' / USARCH / 'trap_table.S',
	'src/trap/trap.c',
	'src/trap/exception.c',
	'src/trap/interrupt.c',
	'src/drivers/tick.c',
	'src/drivers/kbd.c',
	'src/drivers/tty.c',
	'src/drivers/pci.c',
	'src/trap' / USARCH / 'interrupt.c',
)

if USARCH == 'sun4v'
	arch_src += files(
		'src/drivers/niagara.c',
		'src/sun4v/md.c',
	)
endif

if CONFIG_FB
	arch_src += files(
		'src/drivers/scr.c',
	)
endif

if CONFIG_SMP
	arch_src += files(
		'src/smp' / USARCH / 'smp.c',
		'src/smp' / USARCH / 'ipi.c',
	)
endif

if CONFIG_TSB
	arch_src += files(
		'src/mm' / USARCH / 'tsb.c',
	)
endif


_check_headers = [
	'include/arch/istate_struct.h',
	'include/arch/context_struct.h',
]

foreach h : _check_headers
	arch_src += [ autocheck.process(h) ]
endforeach
