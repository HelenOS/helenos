#
# Copyright (c) 2007 Jakub Jermar, Michal Kebrt
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
