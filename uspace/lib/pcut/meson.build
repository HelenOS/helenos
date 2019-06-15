c_args += [ '-D__helenos__', '-Wno-unknown-pragmas' ]

src = files(
	'src/os/helenos.c',
	'src/assert.c',
	'src/helper.c',
	'src/list.c',
	'src/main.c',
	'src/print.c',
	'src/report/report.c',
	'src/report/tap.c',
	'src/report/xml.c',
	'src/run.c',
)

if CONFIG_PCUT_SELF_TESTS
	subdir('tests')
endif
