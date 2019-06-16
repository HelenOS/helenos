
## The at-sign
#
# The `atsign` variable holds the ASCII character representing the at-sign
# ('@') used in various $(AS) constructs (e.g. @progbits). On architectures that
# don't use '@' for starting a comment, `atsign` is merely '@'. However, on
# those that do use it for starting a comment (e.g. arm32), `atsign` must be
# defined as the percentile-sign ('%') in the architecture-dependent files.
#
atsign = '@'

## Some architectures need a particular string at the beginning of assembly files.
as_prolog = ''

kernel_c_args = []
kernel_link_args = []
kernel_includes = []
arch_src = []

subdir('arch' / h_arch)
subdir('genarch')
subdir('generic')
subdir('test')

## Cross-platform assembly to start a symtab.data section
#
symtab_section = '.section symtab.data, "a", ' + atsign + 'progbits;'

kernel_includes += include_directories(
	'generic/include',
	'genarch/include',
	'arch' / h_arch / 'include',
	'..' / 'abi' / 'arch' / h_arch / 'include',
	'..' / 'abi' / 'include',
	'test',
)

kernel_defs = [
	'-imacros', meson.source_root() / 'config.h',
	'-D_HELENOS_SOURCE',
	'-DKERNEL',
	'-DRELEASE=' + HELENOS_RELEASE,
	'-DCOPYRIGHT=' + HELENOS_COPYRIGHT,
	'-DNAME=' + HELENOS_NAME,
	'-D__@0@_BITS__'.format(meson.get_cross_property('bits')),
]

kernel_ldscript = custom_target('_link.ld',
	input: 'arch' / h_arch / '_link.ld.in',
	output: '_link.ld',
	command: [
		cc.cmd_array(),
		kernel_defs,
		'-I' + meson.current_source_dir() / 'arch' / h_arch / 'include',
		'-D__ASSEMBLER__',
		'-D__LINKER__',
		'-E',
		'-P',
		'-x', 'c',
		'@INPUT@',
	],
	capture: true,
)

kernel_link_args += [
	'-Wl,--nmagic',
	'-T', meson.current_build_dir() / '_link.ld',
]

if CONFIG_STRIP_BINARIES
	# TODO: let meson do this
	kernel_link_args += [ '-s' ]
endif

kernel_c_args += kernel_defs + [
	'-ffreestanding',
	# TODO: remove this flag
	'-Wno-cast-function-type',
]

if cc.get_id() == 'clang'
	kernel_c_args += [
		'-fno-stack-protector',
		'-fno-PIC',
		'-mllvm', '-asm-macro-max-nesting-depth=1000',
	]
endif

instrumentables = static_library('instrumentables', instrumentable_src,
	include_directories: kernel_includes,
	implicit_include_directories: false,
	c_args: kernel_c_args + (CONFIG_TRACE ? [ '-finstrument-functions' ] : []),
	pic: false,
)

noninstrumentables = static_library('noninstrumentables', arch_src, genarch_src, generic_src, test_src,
	include_directories: kernel_includes,
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
	command: [ 'echo', as_prolog + symtab_section ],
)

foreach iter : iterations
	is_last = (iter == iterations.length())
	kernel_name = is_last ? 'kernel' : 'kernel.@0@'.format(iter)
	kernel_map_name = kernel_name + '.map'
	kernel_map_path = meson.current_build_dir() / kernel_map_name

	kernel_elf = executable(kernel_name + '.elf', kernel_map_S,
		include_directories: kernel_includes,
		implicit_include_directories: false,
		c_args: kernel_c_args,
		link_args: kernel_c_args + kernel_link_args + [
			'-Wl,-Map,' + kernel_map_path,
		],
		link_depends: kernel_ldscript,
		link_whole: all_kernel_objects,
		install: is_last,
		install_dir: 'boot',
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
			command: [ 'echo', as_prolog + symtab_section + ' .incbin "@INPUT@"' ],
		)
	endif
endforeach

kernel_disasm = custom_target(kernel_name + '.disasm',
	command: [ objdump, '-S', '@INPUT@' ],
	input: kernel_elf,
	output: kernel_name + '.disasm',
	capture: true,
	build_by_default: true,
	# TODO: Add configuration option for installing debug files
	install: false,
	install_dir: 'debug' / 'kernel',
)
