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

# This file sets the architecture-independent compiler flags used throughout
# this repository.
# For architecture-specific flags, see $srcroot/meson/cross/$arch.

add_project_arguments(
	# TODO: Remove from project arguments and only use where necessary.
	# Any change in config forces everything to rebuild due to lack of granularity here.
	'-imacros', meson.build_root() / 'config.h',
	language : [ 'c' ],
)

add_project_link_arguments(
	'-Wl,--gc-sections',
	'-Wl,--warn-common',
	'-Wl,--fatal-warnings',
	'-Wl,-z,text',
	language : [ 'c', 'cpp' ],
)

# TODO: enable more warnings
# FIXME: -fno-builtin-strftime works around seemingly spurious format warning.
# We should investigate what's going on there.

extra_common_flags = [
	'-O' + OPTIMIZATION,
	'-fexec-charset=UTF-8',
	'-finput-charset=UTF-8',

	'-D_HELENOS_SOURCE',

	'-Wa,--fatal-warnings',
	'-Wl,-z,text',

	'-Wall',
	'-Wextra',
	'-Wwrite-strings',
	'-Wunknown-pragmas',

	'-Wno-unused-parameter',

	'-pipe',

	'-ffunction-sections',
	'-fdata-sections',
	'-fno-common',
	'-fdebug-prefix-map=' + meson.source_root() + '/=',
	'-fdebug-prefix-map=../../=',
]

if cc.get_id() != 'clang'
	# Clang's own headers emit macro redefinition warnings.
	extra_common_flags += '-Wsystem-headers'
endif

if UARCH != 'ia64'
	extra_common_flags += [ '-fvar-tracking-assignments' ]
endif

if CONFIG_DEBUG
	extra_common_flags += [ '-Werror' ]
endif

if CONFIG_LINE_DEBUG
	extra_common_flags += [ '-gdwarf-5', '-g3' ]
endif

extra_cflags = extra_common_flags + [
	'-Wmissing-prototypes',
	'-Werror-implicit-function-declaration',

	'-Wno-missing-braces',
	'-Wno-missing-field-initializers',
	'-Wno-unused-command-line-argument',
	'-Wno-unused-parameter',
	'-Wno-typedef-redefinition',
	'-Wno-clobbered',
	'-Wno-nonnull-compare',

	'-fno-builtin-strftime',
]

if CONFIG_UBSAN
	extra_cflags += '-fsanitize=undefined'
endif

extra_cppflags = extra_common_flags + [
	'-fno-exceptions',
	'-Wno-misleading-indentation',
	'-frtti',
]

w_flags = {
	'c': extra_cflags,
	'cpp': extra_cppflags,
}

# TODO: To remove noise in Meson output, we may want to cut down on
#       the explicitly checked flags and just enable those supported by
#       both gcc and clang unconditionally.

# Process flags. Only sets those that compiler supports.
foreach lang : [ 'c', 'cpp' ]
	extra_cflags = meson.get_compiler(lang).get_supported_arguments(w_flags.get(lang))
	add_project_arguments(extra_cflags, language : [ lang ])
	add_project_link_arguments(extra_cflags, language : [ lang ])
endforeach

# This flag is needed at several places, hence we define it here.
#
# For backwards compatibility we try to detect --no-warn-rwx-segments.
# However, the autodetection done by Meson also results in
# "cannot find entry symbol _start; defaulting to 00000000004000b0"
# thus the option is never supported alone. So when detecting we also
# specify --entry=main so that the stub source provided by Meson is build
# correctly.
ldflags_ignore_rwx_segments = []
if cc.has_link_argument('-Wl,--no-warn-rwx-segments,--entry=main')
    ldflags_ignore_rwx_segments += ['-Wl,--no-warn-rwx-segments']
endif
