#
# SPDX-FileCopyrightText: 2021 Jiri Svoboda
# SPDX-FileCopyrightText: 2019 Jiří Zárevúcky
#
# SPDX-License-Identifier: BSD-3-Clause
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
