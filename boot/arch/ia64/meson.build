BUILD = true
POSTBUILD = 'raw'
boot_image_format = 'elf'
POST_OUTPUT = 'image.boot'

boot_src = files(
	'src/boot.S',
	'src/asm.S',
	'src/main.c',
	'src/sal.c',
	'src/sal_asm.S',
	'src/pal.c',
	'src/pal_asm.S',
	'src/putchar.c',
	'../../genarch/src/efi.c',
	'../../genarch/src/division.c',
	'../../generic/src/balloc.c',
	'../../generic/src/memstr.c',
	'../../generic/src/printf_core.c',
	'../../generic/src/vprintf.c',
	'../../generic/src/printf.c',
	'../../generic/src/str.c',
	'../../generic/src/version.c',
	'../../generic/src/inflate.c',
	'../../generic/src/tar.c',
	'../../generic/src/gzip.c',
	'../../generic/src/kernel.c',
	'../../generic/src/payload.c',
)

if MACHINE == 'ski'
	boot_src += files('src/ski.c')
endif
