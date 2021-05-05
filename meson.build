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

# TODO: Use vcs_tag() to generate version string
# TODO: jobfile
# TODO: lto in uspace
# TODO: fix clang build

project(
    'HelenOS',
    [ 'c', 'cpp' ],
    default_options : ['buildtype=plain', 'c_std=gnu11', 'cpp_std=c++17', 'warning_level=2', 'werror=false', 'b_staticpic=false', 'prefix=/' ],
    meson_version: '>=0.50.1',
)

debug_options = false
debug_shell_scripts = false

subdir('meson/part/tools')
subdir('meson/part/read_config')
subdir('meson/part/compiler_args')
subdir('meson/part/initrd_manifest')
subdir('meson' / 'arch' / UARCH)

## The at-sign
#
# The `atsign` variable holds the ASCII character representing the at-sign
# ('@') used in various $(AS) constructs (e.g. @progbits). On architectures that
# don't use '@' for starting a comment, `atsign` is merely '@'. However, on
# those that do use it for starting a comment (e.g. arm32), `atsign` must be
# defined as the percentile-sign ('%') in the architecture-dependent files.
#
if not is_variable('atsign')
	atsign = '@'
endif

## Some architectures need a particular string at the beginning of assembly files.
if not is_variable('kernel_as_prolog')
	kernel_as_prolog = ''
endif
if not is_variable('uspace_as_prolog')
	uspace_as_prolog = ''
endif

# The following variables are filled in by code in the kernel and uspace subdirs.

# References to the actual binary artefacts that will be part of initrd image.
rd_init_binaries = []
# Installed files.
install_files = []
install_deps = []
# Install script for headers.
# TODO: Make a list of directories and turn into script later.
uspace_lib_install_script_text = []

subdir('kernel')
subdir('uspace')

subdir('meson/part/exports')
subdir('meson/part/initrd')

subdir('boot')

# Copy the final image to the build root,
# and write its name to a text file `image_path`.

if is_variable('POST_INPUT')
	image = custom_target(POST_OUTPUT,
		output: POST_OUTPUT,
		input: POST_INPUT,
		command: [ cp, '@INPUT@', '@OUTPUT@' ],
	)

	custom_target('image_path',
		output: 'image_path',
		input: image,
		command: [ 'echo', '@INPUT@' ],
		capture: true,
	)
else
	custom_target('image_path',
		output: 'image_path',
		command: [ 'echo' ],
		capture: true,
	)
endif

subdir('meson/part/extra_targets')
