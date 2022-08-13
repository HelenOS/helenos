#
# SPDX-FileCopyrightText: 2021 Jiri Svoboda
# SPDX-FileCopyrightText: 2019 Jiří Zárevúcky
#
# SPDX-License-Identifier: BSD-3-Clause
#

arch_uspace_c_args = [ '-D__LE__', '-fno-unwind-tables' ]
arch_kernel_c_args = arch_uspace_c_args + [ '-mconstant-gp', '-mfixed-range=f32-f127' ]
if MACHINE == 'i460GX'
	arch_kernel_c_args += [ '-DI460GX' ]
endif

arch_boot_c_args = [ '-D__LE__', '-fno-unwind-tables', '-mfixed-range=f32-f127', '-mno-pic', '-mno-sdata' ]

arch_uspace_link_args = [ '-nostdlib', '-lgcc' ]
arch_kernel_link_args = [ '-nostdlib', '-Wl,-EL' ]
arch_boot_link_args = []


if MACHINE == 'i460GX'
	rd_essential_drv += [
		'drv/platform/pc',
		'drv/bus/pci/pciintel',
		'drv/bus/isa',
		'drv/char/i8042',
		'drv/hid/xtkbd',
	]

elif MACHINE == 'ski'
	rd_essential_drv += [
		'drv/char/ski-con',
		'drv/platform/ski',
	]

endif

rd_drv += rd_essential_drv
