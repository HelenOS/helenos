#
# SPDX-FileCopyrightText: 2007 Jakub Jermar, Michal Kebrt
#
# SPDX-License-Identifier: BSD-3-Clause
#

arch_src += files(
	'src/start.S',
	'src/asm.S',
	'src/eabi.S',
	'src/exc_handler.S',
	'src/arm32.c',
	'src/machine_func.c',
	'src/context.S',
	'src/dummy.S',
	'src/cpu/cpu.c',
	'src/interrupt.c',
	'src/exception.c',
	'src/userspace.c',
	'src/smc.c',
	'src/debug/stacktrace.c',
	'src/debug/stacktrace_asm.S',
	'src/mm/km.c',
	'src/mm/as.c',
	'src/mm/frame.c',
	'src/mm/page.c',
	'src/mm/tlb.c',
	'src/mm/page_fault.c',
	'src/atomic.c',
	'src/ras.c',
)

if CONFIG_FPU
	arch_src += files('src/fpu_context.c', 'src/fpu.S')
endif

if MACHINE == 'gta02'
	arch_src += files('src/mach/gta02/gta02.c')

elif MACHINE == 'integratorcp'
	arch_src += files('src/mach/integratorcp/integratorcp.c')

elif MACHINE == 'beagleboardxm'
	arch_src += files('src/mach/beagleboardxm/beagleboardxm.c')

elif MACHINE == 'beaglebone'
	arch_src += files('src/mach/beaglebone/beaglebone.c')

elif MACHINE == 'raspberrypi'
	arch_src += files('src/mach/raspberrypi/raspberrypi.c')

endif
