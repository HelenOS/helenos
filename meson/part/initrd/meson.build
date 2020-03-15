#
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

if CONFIG_DEVEL_FILES
	# If devel files are requested, we have to install libgcc.
	# We have to explicitly use gcc for this, because clang only prints
	# file name instead of whole path.
	libgcc = run_command(cc_path / cc_arch + '-helenos-gcc',
		arch_uspace_c_args, '-print-libgcc-file-name',
		check: true,
	).stdout().strip()

	install_files += [[ 'lib', libgcc, 'libgcc.a' ]]
	install_deps += [ files(libgcc) ]
endif

# Collect and install copyright information.
copyrights_file = custom_target('COPYRIGHT',
	output: 'COPYRIGHT',
	command: cpc,
	capture: true,
)

install_files += [[ '', copyrights_file.full_path(), 'COPYRIGHT' ]]
install_deps += [ copyrights_file ]

# Emit the install script.

install_script_text = []

# Copy uspace/dist.
_uspace = meson.source_root() / 'uspace'
install_script_text += 'cp -r -L -T -u "@0@/dist" "${DESTDIR}"'.format(_uspace)

# Copy uspace/overlay
install_script_text += 'if ls @0@/overlay/* >/dev/null 2>/dev/null; then'.format(_uspace)
install_script_text += 'cp -r -L @0@/overlay/* "${DESTDIR}"'.format(_uspace)
install_script_text += 'fi'


foreach f : install_files
	_cmd = 'mkdir -p "${DESTDIR}@0@" && cp -L -T "@1@" "${DESTDIR}@0@/@2@"'
	install_script_text += _cmd.format(f[0], f[1], f[2])
endforeach

if CONFIG_DEVEL_FILES
	install_script_text += uspace_lib_devel_install_script_text
endif

install_devel_script = configure_file(
	configuration: { 'text' : '\n'.join(uspace_lib_devel_install_script_text) },
	input: 'install_devel.sh.in',
	output: 'install_devel.sh',
)

install_script_text += uspace_lib_install_script_text

install_script = configure_file(
	configuration: { 'text' : '\n'.join(install_script_text) },
	input: 'install.sh.in',
	output: 'install.sh',
)

# Build up dist

dist_dir = meson.build_root() / 'dist/'

run_target('devel-headers',
	command: [ sh, install_devel_script, dist_dir ],
)

dist = custom_target('DIST',
	output: 'dist.tag',
	input: [ install_script, install_deps ],
	command: [ sh, '@INPUT0@', '@OUTPUT@', dist_dir ],
	build_always_stale: true,
)

# Build initrd image

if RDFMT == 'tmpfs'
	initrd_cmd = [ 'tar', '-c', '-f', '@OUTPUT@', '-C', dist_dir, '.' ]
elif RDFMT == 'fat'
	initrd_cmd = [ mkfat, '1048576', dist_dir, '@OUTPUT@' ]
elif RDFMT == 'ext4fs'
	initrd_cmd = [ mkext4, '1048576', dist_dir, '@OUTPUT@' ]
else
	error('Unknown RDFMT: ' + RDFMT)
endif

initrd_img = custom_target('initrd.img',
	output: 'initrd.img',
	input: dist,
	command: initrd_cmd,
)

rd_init_binaries += [[ initrd_img, 'boot/initrd.img' ]]
