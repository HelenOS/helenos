#
# SPDX-FileCopyrightText: 2015 Petr Pavlu
#
# SPDX-License-Identifier: BSD-3-Clause
#

BUILD = true

if MACHINE == 'virt'
	POST_OUTPUT = 'image.iso'
	POSTBUILD = 'grub'
	GRUB_LOADER = 'chainloader'
endif

if MACHINE == 'hikey960'
	POSTBUILD = 'raw'
	POST_OUTPUT = 'image.boot'
endif

# Request binary output. The ARM64 port manually prepares the .text
# section to look as a PE file than can be loaded on EFI systems.
boot_image_format = 'binary'

boot_src = files(
	'src/asm.S',
	'src/main.c',
	'src/relocate.c',
	'../../genarch/src/efi.c',
	'../../generic/src/gzip.c',
	'../../generic/src/inflate.c',
	'../../generic/src/kernel.c',
	'../../generic/src/memstr.c',
	'../../generic/src/payload.c',
	'../../generic/src/printf.c',
	'../../generic/src/printf_core.c',
	'../../generic/src/str.c',
	'../../generic/src/tar.c',
	'../../generic/src/version.c',
	'../../generic/src/vprintf.c',
)
