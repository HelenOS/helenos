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

atsign = '%'

_cpudef = '-mcpu=' + '-'.join(PROCESSOR.split('_'))


arch_uspace_c_args = [
	_cpudef,
	'-D__LE__',
	'-fno-omit-frame-pointer',
	'-mapcs-frame',
	'-ffixed-r9',
	'-mtp=soft',
]

arch_kernel_c_args = arch_uspace_c_args + [ '-mno-unaligned-access', '-mfpu=vfpv3' ]
arch_kernel_c_args += cc.get_supported_arguments(['-mgeneral-regs-only' ])
arch_kernel_link_args = [ '-nostdlib', '-Wl,-z,max-page-size=0x1000' ]

if CONFIG_FPU
    # Need to use softfp because our libgcc.a uses it too
    # softfp - generate code with soft-float calling conventions but allow use
    # of "hard" float instructions
    arch_uspace_c_args += [ '-mfloat-abi=softfp' ]
endif
arch_uspace_link_args = [ '-nostdlib', '-lgcc', '-Wl,-z,max-page-size=0x1000' ]

arch_boot_c_args = arch_kernel_c_args
arch_boot_link_args = arch_kernel_link_args

rd_essential_drv += [
	'drv/bus/usb/ehci',
	'drv/bus/usb/ohci',
	'drv/bus/usb/usbdiag',
	'drv/bus/usb/usbflbk',
	'drv/bus/usb/usbhub',
	'drv/bus/usb/usbmid',
	'drv/block/usbmast',
	'drv/hid/usbhid',
]

if MACHINE == 'gta02'
	rd_essential += [
		'srv/hid/s3c24xx_ts',
		'srv/hw/char/s3c24xx_uart',
	]

elif MACHINE == 'beagleboardxm' or MACHINE == 'beaglebone'
	rd_essential_drv += [
		'drv/platform/amdm37x',
		'drv/fb/amdm37x_dispc',
	]

elif MACHINE == 'integratorcp'
	rd_essential_drv += [
		'drv/char/pl050',
		'drv/hid/atkbd',
		'drv/hid/ps2mouse',
		'drv/intctl/icp-ic',
		'drv/platform/icp',
	]
endif

rd_drv += rd_essential_drv
