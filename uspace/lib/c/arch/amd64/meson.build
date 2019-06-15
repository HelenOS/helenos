arch_src += [ autocheck.process('include/libarch/fibril_context.h') ]

arch_src += files(
	'src/entryjmp.S',
	'src/thread_entry.S',
	'src/syscall.S',
	'src/fibril.S',
	'src/tls.c',
	'src/stacktrace.c',
	'src/stacktrace_asm.S',
	'src/rtld/dynamic.c',
	'src/rtld/reloc.c',
)

arch_start_src = files('src/crt0.S')

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
