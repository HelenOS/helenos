_march = '-march=' + '-'.join(PROCESSOR.split('_'))
if PROCESSOR == 'core'
	# FIXME
	_march = '-march=prescott'
endif


arch_uspace_c_args = [
	# FIXME: enabling -march for uspace makes malloc tests crash. Investigate.
	# _march,
	'-D__LE__',
	'-mno-tls-direct-seg-refs',
	'-fno-omit-frame-pointer',
]

arch_kernel_c_args = arch_uspace_c_args + [
	_march,
	'-mno-mmx',
	'-mno-sse',
	'-mno-sse2',
	'-mno-sse3',
	'-mno-3dnow',
]


# TODO: Enable --gc-sections.
arch_kernel_link_args = [ '-nostdlib', '-Wl,--no-gc-sections' ]
arch_uspace_link_args = [ '-nostdlib', '-lgcc' ]


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
