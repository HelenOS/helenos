arch_uspace_c_args = [ '-D__LE__', '-fno-unwind-tables' ]
arch_kernel_c_args = arch_uspace_c_args + [ '-mconstant-gp', '-mfixed-range=f32-f127' ]
if MACHINE == 'i460GX'
	arch_kernel_c_args += [ '-DI460GX' ]
endif


arch_uspace_link_args = [ '-nostdlib', '-lgcc' ]
arch_kernel_link_args = [ '-nostdlib', '-Wl,-EL' ]


if MACHINE == 'i460GX'
	rd_essential += [
		'drv/platform/pc',
		'drv/bus/pci/pciintel',
		'drv/bus/isa',
		'drv/char/i8042',
		'drv/hid/xtkbd',
	]

elif MACHINE == 'ski'
	rd_essential += [
		'drv/char/ski-con',
		'drv/platform/ski',
	]

endif
