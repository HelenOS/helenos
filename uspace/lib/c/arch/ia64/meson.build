arch_src = [ autocheck.process('include/libarch/fibril_context.h') ]

arch_src += files(
	'src/entryjmp.S',
	'src/thread_entry.S',
	'src/syscall.S',
	'src/fibril.S',
	'src/tls.c',
	'src/ddi.c',
	'src/stacktrace.c',
	'src/stacktrace_asm.S',
)

arch_start_src = files('src/crt0.S')

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
