#
# SPDX-FileCopyrightText: 2017 Jiri Svoboda
# SPDX-FileCopyrightText: 2019 Jiří Zárevúcky
#
# SPDX-License-Identifier: BSD-3-Clause
#

run_target('config',
	command: [
		sh,
		'-c', 'cd $1 && $2 $3 $4 $5',
		'--',
		meson.build_root(),
		config_py.path(),
		meson.source_root() / 'HelenOS.config',
		meson.source_root() / 'defaults',
		'--mask-platform',
	]
)

# TODO: Check when cross target has changed, since it won't work.

if false
	# TODO: doesn't work because it changes cross target
	run_target('random-config',
		command: [
			sh,
			'-c', 'cd $1 && $2 $3 $4 random',
			'--',
			meson.build_root(),
			config_py.path(),
			meson.source_root() / 'HelenOS.config',
			meson.source_root() / 'defaults',
		]
	)
endif

if doxygen.found()
	_patch_file = files('doxygen.cfg.diff')

	_gen_cfg = custom_target('doxygen.cfg.gen',
		output: 'doxygen.cfg.gen',
		command: [ doxygen, '-g', '@OUTPUT@' ],
	)

	_dox_cfg = custom_target('doxygen.cfg',
		output: 'doxygen.cfg',
		input: [ _gen_cfg, _patch_file ],
		command: [ patch, '-o', '@OUTPUT@', '@INPUT@' ],
	)

	run_target('doxygen', command: [
		sh,
		'-c', 'cd $1 && $2 $3',
		'--',
		meson.source_root() / 'doxygen',
		doxygen.path(),
		_dox_cfg,
	])
endif

if cppcheck.found()
	run_target('cppcheck', command: ['cppcheck', '--project=' +
		join_paths(meson.build_root(), 'compile_commands.json'),
		' -q --force --file-list=*.c* --platform=unix32 --enable=all' ])
endif

# TODO text-xcw
#ifeq ($(CONFIG_DEVEL_FILES),y)
#	export PATH=$$PATH:$(abspath tools/xcw/bin) && $(MAKE) -r -C tools/xcw/demo
#endif

# TODO special target for posix and xcw exports, update coastline for it
