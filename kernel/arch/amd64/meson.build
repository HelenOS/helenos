#
# SPDX-FileCopyrightText: 2005 Martin Decky
# SPDX-FileCopyrightText: 2019 Jiří Zárevúcky
#
# SPDX-License-Identifier: BSD-3-Clause
#

arch_src += files(
	'src/amd64.c',
	'src/asm.S',
	'src/bios/bios.c',
	'src/boot/memmap.c',
	'src/boot/multiboot2.S',
	'src/boot/multiboot.S',
	'src/context.S',
	'src/cpu/cpu.c',
	'src/ddi/ddi.c',
	'src/debug/stacktrace_asm.S',
	'src/debug/stacktrace.c',
	'src/delay.S',
	'src/drivers/i8254.c',
	'src/fpu_context.c',
	'src/interrupt.c',
	'src/kseg.c',
	'src/mm/as.c',
	'src/mm/frame.c',
	'src/mm/km.c',
	'src/mm/page.c',
	'src/mm/tlb.c',
	'src/pm.c',
	'src/proc/scheduler.c',
	'src/proc/task.c',
	'src/proc/thread.c',
	'src/smc.c',
	'src/syscall.c',
	'src/userspace.c',
	'src/vreg.c',
)

if CONFIG_SMP
	arch_src += files(
		'src/smp/apic.c',
		'src/smp/ap.S',
		'src/smp/ipi.c',
		'src/smp/mps.c',
		'src/smp/smp.c',
	)
endif

_check_headers = [
	'include/arch/boot/memmap_struct.h',
	'include/arch/context_struct.h',
	'include/arch/istate_struct.h',
	'include/arch/kseg_struct.h',
]

foreach h : _check_headers
	arch_src += [ autocheck.process(h) ]
endforeach
