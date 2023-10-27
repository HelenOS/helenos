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

kernel_include_dirs = include_directories(
	'generic/include',
	'genarch/include',
	'arch' / KARCH / 'include',
	'..' / 'abi' / 'arch' / KARCH / 'include',
	'..' / 'abi' / 'include',
	'..' / 'common' / 'include',
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
# The kernel is built as ELF but then copied as a blob of bytes and
# the permissions are not relevant anyway (needed for binutils 2.39+).
kernel_link_args += ldflags_ignore_rwx_segments

if CONFIG_LTO
	kernel_link_args += [ '-flto' ]
endif

kernel_c_args = arch_kernel_c_args + kernel_defs + [
	'-ffreestanding',
	'-fdebug-prefix-map=../../kernel/=',
	'-fdebug-prefix-map=../../../kernel/=',

	cc.get_supported_arguments([
		# TODO: remove this flag
		'-Wno-cast-function-type',

		# When accessing specific memory addresses that are below
		# normal page size, the compiler may assume that we actually
		# dereferenced NULL pointer and warns us about that.
		# But in kernel we often need to access these addresses
		# directly hence we need to ignore these warnings.
		#
		# TODO: might make more sense to disable this selectively
		# in specific files (or better yet, for specific lines).
		'--param=min-pagesize=0',
	]),
]

if not CONFIG_LINE_DEBUG
	# Keep the debug info needed to get file names for kernel stack traces.
	kernel_c_args += [ '-gdwarf-5', '-g1', '-gno-statement-frontiers' ]
endif

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

kernel_name = 'kernel.elf'
kernel_map_name = kernel_name + '.map'
kernel_map_path = meson.current_build_dir() / kernel_map_name

kernel_elf = executable(kernel_name,
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

kernel_dbg = custom_target('kernel.dbg',
	output: 'kernel.dbg',
	input: kernel_elf,
	command: [
		objcopy,
		'--only-keep-debug',
		'@INPUT@',
		'@OUTPUT@',
	],
)

kernel_elf_stripped = custom_target(kernel_name + '.stripped',
	output: kernel_name + '.stripped',
	input: kernel_elf,
	command: [
		objcopy,
		'--strip-unneeded',
		'@INPUT@',
		'@OUTPUT@',
	],
)

rd_init_binaries += [[ kernel_elf_stripped, 'boot/kernel.elf' ]]
install_files += [[ 'boot', kernel_elf_stripped.full_path(), 'kernel.elf' ]]
install_deps += [ kernel_elf_stripped ]

if CONFIG_SYMTAB
	rd_init_binaries += [[ kernel_dbg, 'kernel.dbg' ]]
	install_files += [[ 'boot', kernel_dbg.full_path(), 'kernel.dbg' ]]
	install_deps += [ kernel_dbg ]
endif

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
