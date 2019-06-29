BUILD = true
POSTBUILD = 'raw'
POST_OUTPUT = 'image.boot'

if MACHINE == 'msim'
	boot_image_format = 'binary'
elif MACHINE == 'lmalta' or MACHINE == 'bmalta'
	boot_image_format = 'elf'
endif


boot_src = files(
	'src/asm.S',
	'src/main.c',
	'src/putchar.c',
	'../../genarch/src/division.c',
	'../../genarch/src/multiplication.c',
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
