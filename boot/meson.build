#
# Copyright (c) 2006 Martin Decky
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

subdir('arch' / BARCH)

if POSTBUILD == 'uboot'
	boot_image_format = 'binary'
endif

if BUILD
	# Order matters.
	modules = [ 'boot/kernel.elf' ] + rd_init + [ 'boot/initrd.img' ]
	moddeps = []
	name_transform = ''

	# Collect binary references in the correct order.
	foreach m : modules
		found = false
		foreach bin : rd_init_binaries
			if bin[1] == m
				_dep = bin[0]
				_newname = run_command(basename, bin[1], check: true).stdout().strip()
				_oldname = run_command(basename, bin[0].full_path(), check: true).stdout().strip()

				if CONFIG_COMPRESSED_INIT
					_dep = custom_target(_newname + '.gz',
						output: _newname + '.gz',
						input: _dep,
						command: gzip,
						capture: true,
					)
					_newname += '.gz'
					_oldname = _newname
				endif

				moddeps += _dep
				name_transform += 's:.*/@0@:@1@:;'.format(_oldname, _newname)
				found = true
				break
			endif
		endforeach

		if not found
			error('Could not find dependency ' + m)
		endif
	endforeach


	boot_include_dirs = include_directories(
		'generic/include',
		'arch'/BARCH/'include',
		'genarch/include',
		'../abi/arch'/BARCH/'include',
		'../abi/include',
		'../common/include',
	)

	boot_defs = [
		'-imacros', meson.build_root() / 'config.h',
		'-D_HELENOS_SOURCE',
		'-DBOOT',
		'-DHELENOS_RELEASE=' + HELENOS_RELEASE,
		'-DHELENOS_COPYRIGHT=' + HELENOS_COPYRIGHT,
		'-DHELENOS_CODENAME=' + HELENOS_CODENAME,
		'-D__@0@_BITS__'.format(meson.get_cross_property('bits')),
	]

	boot_c_args = arch_boot_c_args + boot_defs + [
		'-ffreestanding',
	]

	boot_link_args = arch_boot_link_args + [
		'-nostdlib',
		'-Wl,--nmagic',
		'-T', meson.current_build_dir()/'_link.ld',
	]

	if cc.get_id() == 'clang'
		boot_c_args += [
			'-fno-stack-protector',
			'-fno-PIC',
		#	'-mllvm', '-asm-macro-max-nesting-depth=1000',
		]
	endif

	boot_files = static_library('bootfiles', boot_src,
		include_directories: boot_include_dirs,
		implicit_include_directories: false,
		c_args: boot_c_args,
		pic: false,
		build_by_default: true,
	)

	# Preprocess linker script using C preprocessor.
	boot_ldscript = custom_target('_link.ld',
		input: 'arch'/KARCH/'_link.ld.in',
		output: '_link.ld',
		command: [
			cc.cmd_array(),
			boot_c_args,
			'-I' + meson.current_source_dir()/'arch'/KARCH/'include',
			'-D__ASSEMBLER__',
			'-D__LINKER__',
			'-E',
			'-P',
			'-x', 'c',
			'@INPUT@',
		],
		capture: true,
		build_by_default: true
	)

	# Create empty object file.
	boot_empty_o = custom_target('empty.o',
		output: 'empty.o',
		command: [
			cc.cmd_array(),
			boot_c_args,
			'-x', 'c',
			'-c',
			'-o', '@OUTPUT@',
			'-',
		],
	)

	boot_comps_tar = custom_target('components.tar',
		input: moddeps,
		output: 'components.tar',
		command: [ tar, '--transform', name_transform ],
	)

	# Add .payload section to the empty object.
	boot_comps_o = custom_target('components.o',
		output: 'components.o',
		input: [ boot_comps_tar, boot_empty_o ],
		command: [
			objcopy,
			'--add-section', '.payload=@INPUT0@',
			'@INPUT1@',
			'@OUTPUT@',
		],
	)

	boot_image_name = 'image.boot'
	boot_image_map_path = meson.current_build_dir()/boot_image_name + '.map'

	boot_elf = executable(boot_image_name + '.elf', boot_comps_o,
		include_directories: boot_include_dirs,
		c_args: boot_c_args,
		link_args: boot_c_args + boot_link_args + [
			'-Wl,-Map,' + boot_image_map_path,
		],
		link_depends: boot_ldscript,
		link_whole: boot_files,
		pie: false,
		build_by_default: false,
	)

	if boot_image_format == 'elf'
		boot_image = boot_elf
	endif

	if boot_image_format == 'binary'
		# Some platforms can't ELF.
		boot_image = custom_target(boot_image_name + '.bin',
			input: boot_elf,
			output: boot_image_name + '.bin',
			command: [ objcopy, '-O', 'binary', '@INPUT@', '@OUTPUT@' ],
		)
	endif
endif

if POSTBUILD == 'raw'
	POST_INPUT = boot_image
endif

if POSTBUILD == 'grub'
	subdir('grub')
endif

if POSTBUILD == 'yaboot'
	subdir('yaboot')
endif

if POSTBUILD == 'silo'
	subdir('silo')
endif

if POSTBUILD == 'uboot'
	IMAGE_NAME = 'HelenOS-' + HELENOS_RELEASE

	POST_INPUT = custom_target('uboot-image',
		output: 'uboot-image.bin',
		input: boot_image,
		command: [
			mkuimage,
			'-name', IMAGE_NAME,
			'-laddr', LADDR,
			'-saddr', SADDR,
			'-ostype', UIMAGE_OS,
			'@INPUT@',
			'@OUTPUT@',
		],
	)
endif
