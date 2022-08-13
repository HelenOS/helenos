#
# SPDX-FileCopyrightText: 2006 Martin Decky
#
# SPDX-License-Identifier: BSD-3-Clause
#

BUILD = true

boot_image_format = 'elf'

if MACHINE == 'gta02'
	LADDR = '0x30008000'
	SADDR = '0x30008000'
	POSTBUILD = 'uboot'
	POST_OUTPUT = 'uImage.bin'
endif

if MACHINE == 'beagleboardxm' or MACHINE == 'beaglebone'
	LADDR = '0x80000000'
	SADDR = '0x80000000'
	POSTBUILD = 'uboot'
	POST_OUTPUT = 'uImage.bin'
endif

if MACHINE == 'raspberrypi'
	LADDR = '0x00008000'
	SADDR = '0x00008000'
	POSTBUILD = 'uboot'
	POST_OUTPUT = 'uImage.bin'
endif

if MACHINE == 'integratorcp'
	POSTBUILD = 'raw'
	POST_OUTPUT = 'image.boot'
endif



boot_src = files(
	'src/asm.S',
	'src/eabi.S',
	'src/main.c',
	'src/mm.c',
	'src/putchar.c',
	'../../genarch/src/division.c',
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
