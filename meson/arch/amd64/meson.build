# TODO: Red Zone is only a problem if we allow interrupts to land on an active
#       stack. Using separate stack for interrupts and ensuring that we never
#       enable interrupts on an interrupt stack would allow us to get rid of
#       the argument.

arch_uspace_c_args = [
	'-D__LE__',
	'-fno-omit-frame-pointer',
]

arch_kernel_c_args = arch_uspace_c_args + [
	'-mno-sse',
	'-mno-sse2',
	'-mcmodel=' + MEMORY_MODEL,
	'-mno-red-zone',
	'-fno-unwind-tables',
]

if PROCESSOR == 'opteron'
	arch_kernel_c_args += '-march=opteron'
endif


# TODO: Enable --gc-sections
arch_kernel_link_args = [ '-Wl,-z,max-page-size=0x1000', '-nostdlib', '-Wl,--no-gc-sections' ]
arch_uspace_link_args = [ '-Wl,-z,max-page-size=0x1000', '-nostdlib', '-lgcc' ]


rd_essential += [
	'app/edit',
	'app/mixerctl',
	'app/wavplay',

	'srv/audio/hound',

	'drv/intctl/apic',
	'drv/intctl/i8259',
	'drv/platform/pc',
	'drv/block/ata_bd',
	'drv/bus/pci/pciintel',
	'drv/bus/isa',
	'drv/audio/sb16',
	'drv/char/i8042',
	'drv/hid/ps2mouse',
	'drv/hid/xtkbd',
]
