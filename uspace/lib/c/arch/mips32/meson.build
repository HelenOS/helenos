arch_src += [ autocheck.process('include/libarch/fibril_context.h') ]

arch_src += files(
	'src/entryjmp.S',
	'src/thread_entry.S',
	'src/syscall.c',
	'src/fibril.S',
	'src/tls.c',
	'src/stacktrace.c',
	'src/stacktrace_asm.S',
)

as_prolog = '.module softfloat;.abicalls;'

arch_start_src = files('src/crt0.S')

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
