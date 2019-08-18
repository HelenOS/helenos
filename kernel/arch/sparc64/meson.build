#
# Copyright (c) 2005 Martin Decky
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
