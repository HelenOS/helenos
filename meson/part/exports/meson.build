#
# SPDX-FileCopyrightText: 2019 Jiří Zárevúcky
#
# SPDX-License-Identifier: BSD-3-Clause
#

## Generate config.mk, config.sh

# Get the directory where the compiler resides.

cc_fullname = run_command(which, meson.get_compiler('c').cmd_array()[0].split(' ')[0], check: true).stdout().strip()
cc_path = run_command(dirname, cc_fullname, check: true).stdout().strip()

message(['Compiler directory is:', cc_path])

export_cppflags = [
	'-isystem', '$(HELENOS_EXPORT_ROOT)/include/libposix',
	'-isystem', '$(HELENOS_EXPORT_ROOT)/include/libc',
	'-idirafter', '$(HELENOS_EXPORT_ROOT)/include',
]

export_ldflags = [
	'-nostdlib',
	'-L$(HELENOS_EXPORT_ROOT)/lib',
	'-Wl,--whole-archive',
	'$(HELENOS_EXPORT_ROOT)/lib/libstartfiles.a',
	'-Wl,--no-whole-archive',
]

export_ldlibs = [
	'-Wl,--as-needed',
	'-lposix',
	'-lmath',
	'-lc',
	'-lgcc',
	'-Wl,--no-as-needed',
]

cc_arch = meson.get_cross_property('cc_arch')

conf_data = configuration_data({
	'HELENOS_OVERLAY_PATH' : meson.source_root() / 'uspace/overlay',
	'HELENOS_CROSS_PATH' : cc_path,
	'HELENOS_ARCH' : cc_arch,
	'HELENOS_CFLAGS' : ' '.join(arch_uspace_c_args),
	'HELENOS_CXXFLAGS' : ' '.join(arch_uspace_c_args),
	'HELENOS_CPPFLAGS' : ' '.join(export_cppflags),
	'HELENOS_LDFLAGS' : ' '.join(export_ldflags),
	'HELENOS_LDLIBS' : ' '.join(export_ldlibs),
	'HELENOS_TARGET' : cc_arch + '-helenos',
})

config_mk = configure_file(
	input: 'config.mk.in',
	output: 'config.mk',
	configuration: conf_data,
)

config_sh = custom_target('config.sh',
	input: config_mk,
	output: 'config.sh',
	command: [ sed, 's:$(HELENOS_EXPORT_ROOT):${HELENOS_EXPORT_ROOT}:g', '@INPUT@' ],
	capture: true,
)
