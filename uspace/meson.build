#
# Copyright (c) 2005 Martin Decky
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# FIXME: somehow disabling link map makes tools/mkext4.py crash
link_map = true
disassemble = CONFIG_LINE_DEBUG
install_nonessential_data = not CONFIG_BAREBONE
# TODO: Allow installing debug files.
# This is currently disabled due to boot image size restrictions.
install_debug_files = false

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

if CONFIG_BAREBONE
	drv_list = rd_essential_drv
else
	drv_list = rd_drv
endif

foreach drv : drvs
	_basename = run_command(basename, drv, check: true).stdout().strip()
	_dirname = run_command(dirname, drv, check: true).stdout().strip()

	dirs += {
		'subdir': 'drv' / drv,
		'installdir': 'drv' / _basename,
	}

	# Install driver metadata.
	if drv_list.contains('drv' / drv)
		_src = meson.current_source_dir() / 'drv' / drv / _basename + '.ma'
		_dstdir = 'drv' / _basename
		install_files += [[ _dstdir, _src, _basename + '.ma' ]]
		install_deps += files(_src)
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
	platform_specific = false

	subdir(appdirs.get('subdir'))

	dir = appdirs.get('subdir')
	installdir = appdirs.get('installdir')

	is_drv = (dir.split('/')[0] == 'drv')

	if is_drv
		# Drivers are installed based on rd_[essential_]drv list
		install = drv_list.contains(dir)
	else
		#
		# Servers and applications are installed all (unless
		# platform-specific) or based on rd_essential in case
		# of barebone build or platform-specific
		#
		install = (not CONFIG_BAREBONE and not platform_specific) \
		    or rd_essential.contains(dir)
	endif

	if install
		# Install data files, if any.
		foreach _f : installed_data
			_dstdir = _f.get('dir')
			_src = meson.current_source_dir() / dir / _f.get('name')
			install_files += [[ _dstdir, _src, _f.get('name') ]]
			install_deps += files(_src)
		endforeach
	endif

	# basename/dirname will be is useful later.
	_basename = run_command(basename, dir, check: true).stdout().strip()
	_dirname = run_command(dirname, dir, check: true).stdout().strip()

	# Extra linker flags

	# TODO: only strip after disassembly
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
			'init': rd_init.contains(dir),
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
			'init': false,
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
	_is_init = tst.get('init')

	if link_map
		# We want linker to generate link map for debugging.
		_ldargs += [ '-Wl,-Map,' + _full_build_name + '.map' ]
	endif

	_bin = executable(_build_name, _src,
		include_directories: tst.get('includes'),
		dependencies: tst.get('dependencies'),
		link_whole: libstartfiles,
		c_args: arch_uspace_c_args + tst.get('c_args'),
		cpp_args: arch_uspace_c_args + tst.get('c_args'),
		link_args: arch_uspace_c_args + arch_uspace_link_args + _ldargs,
		implicit_include_directories: true,
		build_by_default: true,
	)

	if _is_init
		rd_init_binaries += [[ _bin, _full_install_path ]]
	endif

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
