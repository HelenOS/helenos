## Test sources
#

test_src = []

if CONFIG_TEST
	test_src += files(
		'test.c',
		'atomic/atomic1.c',
		'fault/fault1.c',
		'mm/falloc1.c',
		'mm/falloc2.c',
		'mm/mapping1.c',
		'mm/slab1.c',
		'mm/slab2.c',
		'synch/semaphore1.c',
		'synch/semaphore2.c',
		'print/print1.c',
		'print/print2.c',
		'print/print3.c',
		'print/print4.c',
		'print/print5.c',
		'thread/thread1.c',
	)

	if h_arch == 'mips32'
		test_src += files('debug/mips1.c')
	else
		test_src += files('debug/mips1_skip.c')
	endif

	if h_arch == 'ia64'
		test_src += files('mm/purge1.c')
	else
		test_src += files('mm/purge1_skip.c')
	endif
endif
