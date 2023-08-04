#
# Copyright (c) 2017 Jiri Svoboda
# Copyright (c) 2019 Jiří Zárevúcky
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

run_target('config',
	command: [
		sh,
		'-c', 'cd $1 && $2 $3 $4 $5',
		'--',
		meson.build_root(),
		config_py.full_path(),
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
			config_py.full_path(),
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
		doxygen.full_path(),
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
