#
# SPDX-FileCopyrightText: 2005 Martin Decky
# SPDX-FileCopyrightText: 2007 Jakub Jermar
#
# SPDX-License-Identifier: BSD-3-Clause
#


if UARCH == 'ia64'
	link_args += [ '-T', meson.current_source_dir() / 'elf64_ia64_loader.x' ]
else
	link_args += '-Wl,--Ttext-segment=0x70000000'
endif

src = files('main.c')
