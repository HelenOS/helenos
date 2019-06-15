self_tests = [
	[ 'abort', files('abort.c') ],
	[ 'asserts', files('asserts.c') ],
	[ 'beforeafter', files('beforeafter.c') ],
	[ 'errno', files('errno.c') ],
	[ 'inithook', files('inithook.c') ],
	[ 'manytests', files('manytests.c') ],
	[ 'multisuite', files('suite_all.c', 'suite1.c', 'suite2.c', 'tested.c') ],
	[ 'preinithook', files('inithook.c') ],
	[ 'printing', files('printing.c') ],
	[ 'simple', files('simple.c', 'tested.c') ],
	[ 'skip', files('skip.c') ],
	[ 'suites', files('suites.c', 'tested.c') ],
	[ 'teardownaborts', files('teardownaborts.c') ],
	[ 'teardown', files('teardown.c', 'tested.c') ],
	[ 'testlist', files('testlist.c') ],
	[ 'timeout', files('timeout.c') ],
	[ 'xmlreport', files('xmlreport.c', 'tested.c') ],
]

foreach t : self_tests
		testfiles += [ {
			'name': 'pcut-' + t[0],
			'src': t[1],
			'includes': include_directories('../include'),
		} ]
endforeach
