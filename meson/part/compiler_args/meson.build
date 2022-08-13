#
# SPDX-FileCopyrightText: 2019 Jiří Zárevúcky
#
# SPDX-License-Identifier: BSD-3-Clause
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

	'-Wall',
	'-Wextra',
	'-Werror-implicit-function-declaration',
	'-Wwrite-strings',
	'-Wunknown-pragmas',

	'-Wno-unused-parameter',

	'-pipe',

	'-ffunction-sections',
	'-fno-common',
	'-fdebug-prefix-map=' + meson.source_root() + '=.',
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
	extra_common_flags += [ '-gdwarf-4', '-g3' ]
endif

extra_cflags = extra_common_flags + [
	'-Wmissing-prototypes',

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
