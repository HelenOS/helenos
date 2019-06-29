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


kernel_as_prolog = '.module softfloat;'
uspace_as_prolog = '.module softfloat;.abicalls;'


if MACHINE == 'bmalta' or MACHINE == 'lmalta'
	rd_essential += [
		'drv/platform/malta',
		'drv/intctl/i8259',
		'drv/block/ata_bd',
		'drv/bus/pci/pciintel',
		'drv/bus/isa',
		'drv/char/i8042',
		'drv/char/ns8250',
		'drv/hid/ps2mouse',
		'drv/hid/xtkbd',
	]

elif MACHINE == 'msim'
	rd_essential += [
		'drv/platform/msim',
		'drv/block/ddisk',
		'drv/char/msim-con',
	]

endif
