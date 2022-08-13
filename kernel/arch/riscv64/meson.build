#
# SPDX-FileCopyrightText: 2016 Martin Decky
#
# SPDX-License-Identifier: BSD-3-Clause
#

arch_src += files(
	'src/boot/boot.S',
	'src/asm.S',
	'src/context.S',
	'src/debug/stacktrace.c',
	'src/drivers/ucb.c',
	'src/proc/scheduler.c',
	'src/proc/task.c',
	'src/proc/thread.c',
	'src/riscv64.c',
	'src/userspace.c',
	'src/smc.c',
	'src/cpu/cpu.c',
	'src/mm/km.c',
	'src/mm/as.c',
	'src/mm/frame.c',
	'src/mm/page.c',
	'src/mm/tlb.c',
)


_check_headers = [
	'include/arch/istate_struct.h',
	'include/arch/context_struct.h',
]

foreach h : _check_headers
	arch_src += [ autocheck.process(h) ]
endforeach
