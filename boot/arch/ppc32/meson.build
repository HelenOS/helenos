BUILD = true
POSTBUILD = 'yaboot'
POST_OUTPUT = 'image.iso'
boot_image_format = 'elf'

boot_src = files(
	'src/asm.S',
	'src/main.c',
	'src/ofw.c',

	'../../genarch/src/ofw.c',
	'../../genarch/src/ofw_tree.c',
	'../../genarch/src/division.c',
	'../../genarch/src/multiplication.c',
	'../../generic/src/balloc.c',
	'../../generic/src/memstr.c',
	'../../generic/src/printf_core.c',
	'../../generic/src/vprintf.c',
	'../../generic/src/printf.c',
	'../../generic/src/str.c',
	'../../generic/src/version.c',
	'../../generic/src/inflate.c',
	'../../generic/src/gzip.c',
	'../../generic/src/tar.c',
	'../../generic/src/kernel.c',
	'../../generic/src/payload.c',
)
