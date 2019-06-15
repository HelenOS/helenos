# FIXME: somehow disabling link map makes tools/mkext4.py crash
link_map = true
disassemble = CONFIG_LINE_DEBUG
install_nonessential_data = not CONFIG_BAREBONE
# TODO: Allow installing debug files.
# This is currently disabled due to boot image size restrictions.
install_debug_files = false

rd_essential = []

subdir('lib')
subdir('app')
subdir('srv')
subdir('drv')

init = [
	'app/init',
	'srv/bd/rd',
	'srv/fs/' + RDFMT,
	'srv/loader',
	'srv/locsrv',
	'srv/logger',
	'srv/ns',
	'srv/vfs',
]

rd_essential += init

rd_essential += [
	'app/bdsh',
	'app/getterm',
	'app/kio',

	'srv/devman',
	'srv/fs/locfs',
	'srv/hid/input',
	'srv/hid/output',
	'srv/hid/compositor',
	'srv/hid/console',
	'srv/klog',

	'drv/root/root',
	'drv/root/virt',
	'drv/fb/kfb',
]

if CONFIG_FB
	rd_essential += [
		'app/vlaunch',
		'app/vterm',
	]
endif

dirs = []

foreach app : apps
	dirs += {
		'subdir': join_paths('app', app),
		'installdir': 'app',
	}
endforeach

foreach srv : srvs
	_dirname = run_command(dirname, srv, check: true).stdout().strip()

	dirs += {
		'subdir': join_paths('srv', srv),
		'installdir': _dirname == '.' ? 'srv' : ('srv' / _dirname),
	}
endforeach

foreach drv : drvs
	_basename = run_command(basename, drv, check: true).stdout().strip()
	_dirname = run_command(dirname, drv, check: true).stdout().strip()

	dirs += {
		'subdir': 'drv' / drv,
		'installdir': 'drv' / _basename,
	}

	# Install driver metadata.
	if not CONFIG_BAREBONE or rd_essential.contains('drv' / drv)
		install_data('drv' / drv / _basename + '.ma', install_dir: 'drv' / _basename)
	endif
endforeach

bin_targets = []

foreach appdirs : dirs
	src = []
	test_src = []
	includes = []
	deps = []
	c_args = []
	link_args = []
	language = 'c'

	subdir(appdirs.get('subdir'))

	dir = appdirs.get('subdir')
	installdir = appdirs.get('installdir')

	install = not CONFIG_BAREBONE or rd_essential.contains(dir)

	# basename/dirname will be is useful later.
	_basename = run_command(basename, dir, check: true).stdout().strip()
	_dirname = run_command(dirname, dir, check: true).stdout().strip()

	# Extra linker flags

	# TODO: let meson do this on install instead, so that disassembly works
	if CONFIG_STRIP_BINARIES
		link_args += [ '-s' ]
	endif

	# Init binaries need to always be linked statically.
	static_build = (not CONFIG_USE_SHARED_LIBS) or init.contains(dir)

	# Add the corresponding standard libraries to dependencies.

	deps += [ 'c' ]

	if language == 'cpp'
		deps += 'cpp'
	endif

	# Binaries in the 'drv' subdirectory link libdrv by default.

	is_drv = (dir.split('/')[0] == 'drv')

	if is_drv
		deps += [ 'drv' ]
	endif

	# Convert strings to dependency objects

	_deps = []
	foreach s : deps
		_deps += get_variable('lib' + s).get(static_build ? 'static' : 'any')
	endforeach

	# Build executable

	if src.length() > 0
		bin_targets += {
			'src': src,
			'dirname': _dirname,
			'basename': _basename,
			'install': install,
			'install_dir': installdir,
			'includes': includes,
			'dependencies': _deps,
			'c_args': c_args,
			'link_args': link_args + (static_build ? [ '-static' ] : []),
		}
	endif

	# Build test executable, if any

	if test_src.length() > 0
		bin_targets += {
			'src': test_src,
			'dirname': _dirname,
			'basename': 'test-' + installdir.underscorify() + (is_drv ? '' : ('_' + _basename)),
			'install': install and CONFIG_PCUT_TESTS,
			'install_dir': 'test',
			'includes': includes,
			'dependencies': [ _deps, libpcut.get('any') ],
			'c_args': c_args,
			'link_args': link_args,
		}
	endif
endforeach

foreach tst : bin_targets
	_ldargs = tst.get('link_args')
	_src = tst.get('src')

	_install_dir = tst.get('install_dir').split('/')
	_install_dir += tst.get('basename')
	_install = tst.get('install')

	if _install
		_install_name = 'install@' + '$'.join(_install_dir)
	else
		_install_name = '_'.join(_install_dir)
	endif

	if _install and install_debug_files
		_debug_name = 'install@' + '$'.join([ 'debug' ] + _install_dir)
	else
		_debug_name = '_'.join(_install_dir)
	endif

	if link_map
		# We want linker to generate link map for debugging.
		_ldargs += [ '-Wl,-Map,' + meson.current_build_dir() / _debug_name + '.map' ]
	endif

	_bin = executable(_install_name, _src,
		include_directories: tst.get('includes'),
		dependencies: tst.get('dependencies'),
		objects: startfiles,
		c_args: tst.get('c_args'),
		link_args: _ldargs,
		implicit_include_directories: false,
		install: false,
		#install_dir: tst.get('install_dir'),
		build_by_default: true,
	)

	if disassemble
		custom_target(_debug_name + '.disasm',
			command: [ objdump, '-S', '@INPUT@' ],
			input: _bin,
			output: _debug_name + '.disasm',
			capture: true,
			install: false,
			#install_dir: 'debug' / tst.get('install_dir'),
			build_by_default: true,
		)
	endif
endforeach
