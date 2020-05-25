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

if GRUB_ARCH == 'pc'
	MULTIBOOT_CMD = 'multiboot'
	MODULE_CMD = 'module'
	INSMODS = [ 'insmod vbe', 'insmod vga' ]
elif GRUB_ARCH == 'efi'
	MULTIBOOT_CMD = 'multiboot2'
	MODULE_CMD = 'module2'
	INSMODS = [ 'insmod efi_gop', 'insmod efi_uga' ]
endif

# TODO
install_snippet = 'mkdir -p ${DIST_DIR}/`dirname @1@`; cp @0@ ${DIST_DIR}/@1@'
dist_dirname = 'grub_dist'

if GRUB_LOADER == 'multiboot'
	build_dist_text = []
	build_dist_deps = []
	foreach init : rd_init_binaries
		target = 'boot/' + run_command(basename, init[1], check: true).stdout().strip()

		build_dist_text += install_snippet.format(init[0].full_path(), target)
		build_dist_deps += init[0]
	endforeach

	LOADS = [ 'echo \'Loading kernel\'' ]
	LOADS += [ MULTIBOOT_CMD + ' /boot/kernel.elf' ]

	MODULES = rd_init + [ 'boot/initrd.img' ]

	foreach module : MODULES
		module = '/boot/' + run_command(basename, module, check: true).stdout().strip()
		LOADS += 'echo \'Loading @0@\''.format(module)
		LOADS += '@0@ @1@ @1@'.format(MODULE_CMD, module)
	endforeach
endif

if GRUB_LOADER == 'chainloader'
	# init binaries are already part of the chainloaded boot image.
	build_dist_text = [ install_snippet.format(boot_image.full_path(), 'boot/' + boot_image_name) ]
	build_dist_deps = [ boot_image ]

	LOADS = [
		'echo \'Loading ' + boot_image_name + '\'',
		'chainloader /boot/' + boot_image_name,
		'boot',
	]
endif

build_dist_sh = configure_file(
	input: 'build_dist.sh.in',
	output: 'build_dist.sh',
	configuration: { 'text' : '\n'.join(build_dist_text) },
)


grub_cfg = configure_file(
	input: 'grub.cfg.in',
	output: 'grub.cfg',
	configuration: {
		'INSMODS' : '\n'.join(INSMODS),
		'RELEASE' : HELENOS_RELEASE,
		'LOADS'   : '\n'.join(LOADS),
	},
)

grub_dir = meson.current_source_dir() / BARCH + '-' + GRUB_ARCH

dist_dir = custom_target(dist_dirname,
	output: dist_dirname,
	input: [ build_dist_sh, grub_cfg, build_dist_deps ],
	command: [ sh, '@INPUT0@', '@OUTPUT@', grub_dir, '@INPUT1@' ],
)


# Create .iso image.

grub_image = 'boot/grub' / GRUB_ARCH + '.img'

if GRUB_ARCH == 'pc'
	genisoimage_args = [ '-eltorito-boot', grub_image, '-no-emul-boot', '-boot-info-table' ]
elif GRUB_ARCH == 'efi'
	if genisoimage_type == 'mkisofs'
		genisoimage_args = [ '-eltorito-platform', 'efi', '-eltorito-boot', grub_image, '-no-emul-boot' ]
	else
		genisoimage_args = [ '--efi-boot', grub_image ]
	endif
endif

image_iso = custom_target('image.iso',
	output: 'image.iso',
	input: dist_dir,
	command: [
		genisoimage,
		'-J',
		'-r',
		'-input-charset', 'utf-8',
		'-V', 'HelenOS-CD',
		genisoimage_args,
		'-o', '@OUTPUT@',
		'@INPUT@',
	],
)

POST_INPUT = image_iso
