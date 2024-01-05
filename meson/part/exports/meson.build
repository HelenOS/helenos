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

run_target('export-dev',
    command: [
        sh,
        meson.source_root() / 'meson' / 'part' / 'exports' / 'copy-export.sh',
    ] + [
        'config', config_mk, 'config.mk',
        'config', config_sh, 'config.sh',
    ] + exported_devel_files
)
