# FIXME: somehow disabling link map makes tools/mkext4.py crash
link_map = true
disassemble = CONFIG_LINE_DEBUG
install_nonessential_data = not CONFIG_BAREBONE
# TODO: Allow installing debug files.
# This is currently disabled due to boot image size restrictions.
install_debug_files = false

install_files = []
install_deps = []

subdir('lib')
subdir('app')
subdir('srv')
subdir('drv')

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
		_src = meson.current_source_dir() / 'drv' / drv / _basename + '.ma'
		_dstdir = 'drv' / _basename
		install_files += [[ _dstdir, _src, _basename + '.ma' ]]
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
	installed_data = []

	subdir(appdirs.get('subdir'))

	dir = appdirs.get('subdir')
	installdir = appdirs.get('installdir')

	install = not CONFIG_BAREBONE or rd_essential.contains(dir)

	if install
		# Install data files, if any.
		foreach _f : installed_data
			_dstdir = _f.get('dir')
			_src = meson.current_source_dir() / dir / _f.get('name')
			install_files += [[ _dstdir, _src, _f.get('name') ]]
		endforeach
	endif

	# basename/dirname will be is useful later.
	_basename = run_command(basename, dir, check: true).stdout().strip()
	_dirname = run_command(dirname, dir, check: true).stdout().strip()

	# Extra linker flags

	# TODO: let meson do this on install instead, so that disassembly works
	if CONFIG_STRIP_BINARIES
		link_args += [ '-s' ]
	endif

	# Init binaries need to always be linked statically.
	static_build = (not CONFIG_USE_SHARED_LIBS) or rd_init.contains(dir)

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

	_install = tst.get('install')
	_install_dir = tst.get('install_dir')
	_basename = tst.get('basename')
	_full_install_path = _install_dir / _basename
	_build_name = _full_install_path.underscorify()
	_full_build_name = meson.current_build_dir() / _build_name

	if link_map
		# We want linker to generate link map for debugging.
		_ldargs += [ '-Wl,-Map,' + _full_build_name + '.map' ]
	endif

	_bin = executable(_build_name, _src,
		include_directories: tst.get('includes'),
		dependencies: tst.get('dependencies'),
		objects: startfiles,
		c_args: arch_uspace_c_args + tst.get('c_args'),
		cpp_args: arch_uspace_c_args + tst.get('c_args'),
		link_args: arch_uspace_c_args + arch_uspace_link_args + _ldargs,
		implicit_include_directories: false,
		build_by_default: true,
	)

	if disassemble
		_disasm = custom_target(_build_name + '.disasm',
			command: [ objdump, '-S', '@INPUT@' ],
			input: _bin,
			output: _build_name + '.disasm',
			capture: true,
			build_by_default: true,
		)
	endif

	if _install
		# Due to certain quirks of our build, executables need to be built with a different name than what they are installed with.
		# Meson doesn't support renaming installed files (at least not as of mid-2019) so we do it manually.

		install_files += [[ _install_dir, _full_build_name, _basename ]]
		install_deps += [ _bin ]

		if install_debug_files
			if disassemble
				install_files += [[ 'debug' / _install_dir, _full_build_name + '.disasm', _basename + '.disasm' ]]
				install_deps += [ _disasm ]
			endif
			if link_map
				install_files += [[ 'debug' / _install_dir, _full_build_name + '.map', _basename + '.map' ]]
			endif
		endif
	endif
endforeach

uspace_bin_install_script_text = []

foreach f : install_files
	_cmd = 'mkdir -p "${MESON_INSTALL_DESTDIR_PREFIX}@0@" && cp -L -T "@1@" "${MESON_INSTALL_DESTDIR_PREFIX}@0@/@2@"'
	uspace_bin_install_script_text += _cmd.format(f[0], f[1], f[2])
endforeach

uspace_install_script_text = uspace_lib_install_script_text + uspace_bin_install_script_text

# Emit and register the install script.

configure_file(
	configuration: { 'text' : '\n'.join(uspace_install_script_text) },
	input: 'install.sh.in',
	output: 'install.sh',
)

meson.add_install_script(meson.current_build_dir() / 'install.sh')
