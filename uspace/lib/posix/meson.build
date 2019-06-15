includes += include_directories('include/posix', 'include')
c_args += [ '-fno-builtin', '-D_XOPEN_SOURCE' ]

# TODO

src = files(
	'src/ctype.c',
	'src/dlfcn.c',
	'src/fcntl.c',
	'src/fnmatch.c',
	'src/locale.c',
	'src/pthread/condvar.c',
	'src/pthread/keys.c',
	'src/pthread/mutex.c',
	'src/pthread/threads.c',
	'src/pwd.c',
	'src/signal.c',
	'src/stdio.c',
	'src/stdlib.c',
	'src/string.c',
	'src/strings.c',
	'src/sys/mman.c',
	'src/sys/stat.c',
	'src/sys/wait.c',
	'src/time.c',
	'src/unistd.c',
)

test_src = files(
	'test/main.c',
	'test/stdio.c',
	'test/stdlib.c',
	'test/unistd.c',
)

# TODO: exports
