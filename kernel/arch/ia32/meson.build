#
# SPDX-FileCopyrightText: 2005 Martin Decky
#
# SPDX-License-Identifier: BSD-3-Clause
#

arch_src += files(
	'src/context.S',
	'src/debug/stacktrace.c',
	'src/debug/stacktrace_asm.S',
	'src/delay.S',
	'src/asm.S',
	'src/proc/scheduler.c',
	'src/proc/task.c',
	'src/proc/thread.c',
	'src/bios/bios.c',
	'src/smp/ap.S',
	'src/smp/apic.c',
	'src/smp/mps.c',
	'src/smp/smp.c',
	'src/atomic.S',
	'src/smp/ipi.c',
	'src/ia32.c',
	'src/vreg.c',
	'src/interrupt.c',
	'src/pm.c',
	'src/userspace.c',
	'src/smc.c',
	'src/cpu/cpu.c',
	'src/mm/km.c',
	'src/mm/as.c',
	'src/mm/frame.c',
	'src/mm/page.c',
	'src/mm/tlb.c',
	'src/ddi/ddi.c',
	'src/drivers/i8254.c',
	'src/boot/multiboot.S',
	'src/boot/multiboot2.S',
	'src/boot/memmap.c',
	'src/fpu_context.c',
	'src/syscall.c',
)

_check_headers = [
	'include/arch/istate_struct.h',
	'include/arch/context_struct.h',
	'include/arch/boot/memmap_struct.h',
]

foreach h : _check_headers
	arch_src += [ autocheck.process(h) ]
endforeach
