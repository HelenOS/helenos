#
# SPDX-FileCopyrightText: 2021 Jiri Svoboda
# SPDX-FileCopyrightText: 2019 Jiří Zárevúcky
#
# SPDX-License-Identifier: BSD-3-Clause
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

if CONFIG_FPU
	# This is necessary for kernel too, to allow vmsr insn and fpexc manipulation.
	# Use vfp32 to allow context save/restore of d16-d31 regs.
	arch_uspace_c_args += [ '-mfloat-abi=hard' ]
endif

arch_kernel_c_args = arch_uspace_c_args + [ '-mno-unaligned-access', '-mfpu=vfpv3' ]
arch_kernel_link_args = [ '-nostdlib', '-Wl,-z,max-page-size=0x1000' ]
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
