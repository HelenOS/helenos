arch_src += [ autocheck.process('include/libarch/fibril_context.h') ]

arch_src += files(
	'src/entryjmp.S',
	'src/fibril.S',
	'src/stacktrace.c',
	'src/stacktrace_asm.S',
	'src/syscall.c',
	'src/tls.c',
	'src/thread_entry.S',
)

arch_start_src = files('src/crt0.S')
