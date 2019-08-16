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


arch_src = []

# Fills arch_src.
subdir('arch' / KARCH)

# Defines genarch_src.
subdir('genarch')

# Defines generic_src, instrumentable_src.
subdir('generic')

# Defines test_src
subdir('test')

## Cross-platform assembly to start a symtab.data section
#
symtab_section = '.section symtab.data, "a", ' + atsign + 'progbits;'

kernel_include_dirs = include_directories(
	'generic/include',
	'genarch/include',
	'arch' / KARCH / 'include',
	'..' / 'abi' / 'arch' / KARCH / 'include',
	'..' / 'abi' / 'include',
	'test',
)

kernel_defs = [
	'-imacros', meson.build_root() / 'config.h',
	'-D_HELENOS_SOURCE',
	'-DKERNEL',
	'-DHELENOS_RELEASE=' + HELENOS_RELEASE,
	'-DHELENOS_COPYRIGHT=' + HELENOS_COPYRIGHT,
	'-DHELENOS_CODENAME=' + HELENOS_CODENAME,
	'-D__@0@_BITS__'.format(meson.get_cross_property('bits')),
]

# Preprocess linker script using C preprocessor.
kernel_ldscript = custom_target('_link.ld',
	input: 'arch' / KARCH / '_link.ld.in',
	output: '_link.ld',
	command: [
		cc.cmd_array(),
		arch_kernel_c_args,
		kernel_defs,
		'-I' + meson.current_source_dir() / 'arch' / KARCH / 'include',
		'-D__ASSEMBLER__',
		'-D__LINKER__',
		'-E',
		'-P',
		'-x', 'c',
		'@INPUT@',
	],
	capture: true,
	build_by_default: true,
)

kernel_link_args = arch_kernel_link_args + [
	'-Wl,--nmagic',
	'-T', meson.current_build_dir() / '_link.ld',
]

if CONFIG_LTO
	kernel_link_args += [ '-flto' ]
endif

if CONFIG_STRIP_BINARIES
	# TODO: do this after disassembling
	kernel_link_args += [ '-s' ]
endif

kernel_c_args = arch_kernel_c_args + kernel_defs + [
	'-ffreestanding',
	# TODO: remove this flag
	cc.get_supported_arguments([ '-Wno-cast-function-type' ]),
]

if CONFIG_LTO
	kernel_c_args += [ '-flto' ]
endif

if cc.get_id() == 'clang'
	kernel_c_args += [
		'-fno-stack-protector',
		'-fno-PIC',
		'-mllvm', '-asm-macro-max-nesting-depth=1000',
	]
endif

instrumentables = static_library('instrumentables', instrumentable_src,
	include_directories: kernel_include_dirs,
	implicit_include_directories: false,
	c_args: kernel_c_args + (CONFIG_TRACE ? [ '-finstrument-functions' ] : []),
	pic: false,
)

noninstrumentables = static_library('noninstrumentables', arch_src, genarch_src, generic_src, test_src,
	include_directories: kernel_include_dirs,
	implicit_include_directories: false,
	c_args: kernel_c_args,
	pic: false,
)

all_kernel_objects = [ instrumentables, noninstrumentables ]

# We iterate the build several times to get symbol table right.
# Three times is sufficient to get correct even symbols after symtab.

if CONFIG_SYMTAB
	# Iterate build three times.
	iterations = [ 1, 2, 3 ]

	# Generates symbol table information as an object file.
	genmap = find_program('tools/genmap.py')

	# Symbol table dump needed for genmap.
	kernel_syms = custom_target('kernel_syms.txt',
		input: all_kernel_objects,
		output: 'kernel_syms.txt',
		command: [ objdump, '-t', '@INPUT@' ],
		capture: true,
	)
else
	# Build just once.
	iterations = [ 1 ]
endif

# Empty symbol map for first iteration.
kernel_map_S = custom_target('empty_map.S',
	output: 'empty_map.S',
	capture: true,
	command: [ 'echo', kernel_as_prolog + symtab_section ],
)

foreach iter : iterations
	is_last = (iter == iterations.length())
	kernel_name = 'kernel.@0@.elf'.format(iter)
	kernel_map_name = kernel_name + '.map'
	kernel_map_path = meson.current_build_dir() / kernel_map_name

	kernel_elf = executable(kernel_name, kernel_map_S,
		include_directories: kernel_include_dirs,
		implicit_include_directories: false,
		c_args: kernel_c_args,
		link_args: kernel_c_args + kernel_link_args + [
			'-Wl,-Map,' + kernel_map_path,
		],
		link_depends: kernel_ldscript,
		link_whole: all_kernel_objects,
		pie: false,
	)

	# Generate symbol table if this is not the final iteration.
	if not is_last

		# TODO: Teach kernel to read its own ELF symbol table and get rid of this nonsense.
		# Need to first make sure all architectures (even future ones with dumb bootloaders) can use ELF formatted kernel.

		kernel_map_bin = custom_target(kernel_map_name + '.bin',
			output: kernel_map_name + '.bin',
			input: [ kernel_elf, kernel_syms ],
			command: [ genmap, kernel_map_path, '@INPUT1@', '@OUTPUT@' ],
		)

		kernel_map_S_name = kernel_name + '.map.S'

		kernel_map_S = custom_target(kernel_map_S_name,
			input: kernel_map_bin,
			output: kernel_map_S_name,
			capture: true,
			command: [ 'echo', kernel_as_prolog + symtab_section + ' .incbin "@INPUT@"' ],
		)
	endif
endforeach

rd_init_binaries += [[ kernel_elf, 'boot/kernel.elf' ]]

install_files += [[ 'boot', kernel_elf.full_path(), 'kernel.elf' ]]
install_deps += [ kernel_elf ]

kernel_disasm = custom_target('kernel.elf.disasm',
	command: [ objdump, '-S', '@INPUT@' ],
	input: kernel_elf,
	output: 'kernel.elf.disasm',
	capture: true,
	build_by_default: true,
)

# TODO: Add configuration option for installing debug files
if false
	install_files += [[ 'boot', kernel_disasm.full_path(), 'kernel.elf.disasm' ]]
	install_deps += [ kernel_disasm ]
endif
