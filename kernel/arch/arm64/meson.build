#
# Copyright (c) 2015 Petr Pavlu
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
