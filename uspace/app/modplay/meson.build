#
# SPDX-FileCopyrightText: 2014 Jiri Svoboda
#
# SPDX-License-Identifier: BSD-3-Clause
#

deps = [ 'trackmod', 'hound', 'pcm' ]
src = files('modplay.c')

if install_nonessential_data
	# TODO: install this file somewhere sane
	installed_data += { 'name': 'demo.xm', 'dir': '/' }
endif
