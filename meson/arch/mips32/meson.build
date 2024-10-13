#
# Copyright (c) 2021 Jiri Svoboda
# Copyright (c) 2019 Jiří Zárevúcky
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

if MACHINE == 'msim'
	_march = '-march=r4000'
	_endian = '-D__LE__'
elif MACHINE == 'lmalta'
	_march = '-march=4kc'
	_endian = '-D__LE__'
elif MACHINE == 'bmalta'
	_march = '-march=4kc'
	_endian = '-D__BE__'
else
	error('Unknown machine')
endif

arch_uspace_c_args = [
	_march,
	_endian,
	'-fno-omit-frame-pointer',
	'-msoft-float',
	'-mabi=32',
]

arch_kernel_c_args = arch_uspace_c_args + [
	'-mno-abicalls',
	'-G', '0',
	'-fno-zero-initialized-in-bss',
]

arch_boot_c_args = arch_kernel_c_args

arch_kernel_link_args = [ '-nostdlib' ]
arch_uspace_link_args = [ '-nostdlib', '-lgcc' ]
arch_boot_link_args = []

uspace_as_prolog = '.module softfloat;.abicalls;'

if MACHINE == 'bmalta' or MACHINE == 'lmalta'
	rd_essential_drv += [
		'drv/platform/malta',
		'drv/intctl/i8259',
		'drv/block/isa-ide',
		'drv/bus/pci/pciintel',
		'drv/bus/isa',
		'drv/char/i8042',
		'drv/char/ns8250',
		'drv/hid/ps2mouse',
		'drv/hid/xtkbd',
	]

elif MACHINE == 'msim'
	rd_essential_drv += [
		'drv/platform/msim',
		'drv/block/ddisk',
		'drv/char/msim-con',
	]

endif

rd_drv += rd_essential_drv
