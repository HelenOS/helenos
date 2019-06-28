

# TODO
install_snippet = 'mkdir -p ${DIST_DIR}/`dirname @1@`; cp @0@ ${DIST_DIR}/@1@'
dist_dirname = 'grub_dist'

build_dist_text = []
build_dist_deps = []
foreach init : rd_init_binaries
	target = 'boot/' + run_command(basename, init[1], check: true).stdout().strip()

	build_dist_text += install_snippet.format(init[0].full_path(), target)
	build_dist_deps += init[0]
endforeach

build_dist_sh = configure_file(
	input: 'build_dist.sh.in',
	output: 'build_dist.sh',
	configuration: { 'text' : '\n'.join(build_dist_text) },
)


MODULES = rd_init + [ 'boot/initrd.img' ]

if GRUB_ARCH == 'pc'
	MULTIBOOT_CMD = 'multiboot'
	MODULE_CMD = 'module'
	INSMODS = [ 'insmod vbe', 'insmod vga' ]
elif GRUB_ARCH == 'efi'
	MULTIBOOT_CMD = 'multiboot2'
	MODULE_CMD = 'module2'
	INSMODS = [ 'insmod efi_gop', 'insmod efi_uga' ]
endif

if GRUB_LOADER == 'multiboot'
	LOADS = [ 'echo \'Loading kernel\'' ]
	LOADS += [ MULTIBOOT_CMD + ' /boot/kernel.elf' ]

	foreach module : MODULES
		module = '/boot/' + run_command(basename, module, check: true).stdout().strip()
		LOADS += 'echo \'Loading @0@\''.format(module)
		LOADS += '@0@ @1@ @1@'.format(MODULE_CMD, module)
	endforeach
endif

if GRUB_LOADER == 'chainloader'
	LOADS = [
		'echo \'Loading ' + BOOT_OUTPUT + '\'',
		'chainloader /boot/' + BOOT_OUTPUT,
		'boot',
	]
endif

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
	genisoimage_args = [ '--efi-boot', grub_image ]
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
