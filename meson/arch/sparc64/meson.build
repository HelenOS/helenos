#
# Copyright (c) 2023 Jiri Svoboda
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

arch_uspace_c_args = [ '-D__BE__', '-m64', '-mcmodel=medlow', '-mhard-float', '-m' + QUADFLOAT + '-quad-float' ]

if PROCESSOR == 'sun4v'
	arch_uspace_c_args += [ '-mcpu=niagara', '-mno-vis' ]
else
	arch_uspace_c_args += [ '-mcpu=ultrasparc' ]
endif

if PROCESSOR == 'us' or PROCESSOR == 'us3'
	arch_uspace_c_args += [ '-DSUN4U' ]
elif PROCESSOR == 'sun4v'
	arch_uspace_c_args += [ '-DSUN4V' ]
else
	error('Unknown PROCESSOR: ' + PROCESSOR)
endif


arch_kernel_c_args = [ '-D__BE__', '-m64', '-mcmodel=medlow', '-mcpu=ultrasparc', '-mno-fpu' ]
arch_boot_c_args = arch_kernel_c_args

if PROCESSOR == 'us'
	arch_kernel_c_args += [
		'-DUS',
		'-DSUN4U',
	]
elif PROCESSOR == 'us3'
	arch_kernel_c_args += [
		'-DUS3',
		'-DSUN4U',
	]
elif PROCESSOR == 'sun4v'
	arch_kernel_c_args += [
		'-DSUN4V',
		#MH
		'-DUS',
	]
endif

arch_boot_link_args = []
arch_kernel_link_args = [ '-nostdlib', '-Wl,-no-check-sections' ]
arch_uspace_link_args = [ '-nostdlib', '-lgcc' ]

if PROCESSOR == 'sun4v'
	arch_uspace_link_args += [ '-Wl,-z,max-page-size=0x2000' ]
else
	arch_uspace_link_args += [ '-Wl,-z,max-page-size=0x4000' ]
endif

rd_essential += [
	'app/ofw'
]

if PROCESSOR == 'sun4v'
	rd_essential_drv += [
		'drv/platform/sun4v',
		'drv/char/sun4v-con',
	]
else
	rd_essential_drv += [
		'drv/platform/sun4u',
		'drv/bus/pci/pciintel',
		'drv/bus/isa',
		'drv/intctl/obio',
		'drv/char/ns8250',
		'drv/char/i8042',
		'drv/hid/ps2mouse',
		'drv/hid/xtkbd',
	]
endif

rd_drv += rd_essential_drv
