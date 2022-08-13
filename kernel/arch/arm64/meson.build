#
# SPDX-FileCopyrightText: 2015 Petr Pavlu
#
# SPDX-License-Identifier: BSD-3-Clause
#

arch_src += files(
	'src/arm64.c',
	'src/asm.S',
	'src/context.S',
	'src/cpu/cpu.c',
	'src/debug/stacktrace.c',
	'src/debug/stacktrace_asm.S',
	'src/exception.c',
	'src/fpu.S',
	'src/fpu_context.c',
	'src/interrupt.c',
	'src/machine_func.c',
	'src/mm/as.c',
	'src/mm/frame.c',
	'src/mm/km.c',
	'src/mm/page.c',
	'src/mm/tlb.c',
	'src/smp/ipi.c',
	'src/smp/smp.c',
	'src/start.S',
)

if MACHINE == 'virt'
	arch_src += files('src/mach/virt/virt.c')
endif

if MACHINE == 'hikey960'
	arch_src += files('src/mach/hikey960/hikey960.c')
endif

_check_headers = [
	'include/arch/context_struct.h',
	'include/arch/fpu_context_struct.h',
	'include/arch/istate_struct.h',
]

foreach h : _check_headers
	arch_src += [ autocheck.process(h) ]
endforeach
