#
# SPDX-FileCopyrightText: 2012 Jan Vesely
#
# SPDX-License-Identifier: BSD-3-Clause
#

deps = [ 'drv', 'hound', 'pcm' ]
src = files(
	'dplay.c',
	'drec.c',
	'main.c',
	'wave.c',
)

if install_nonessential_data
	# TODO: install this file somewhere sane
	installed_data += { 'name': 'demo.wav', 'dir': '/' }
endif
