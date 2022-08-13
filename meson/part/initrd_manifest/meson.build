#
# SPDX-FileCopyrightText: 2019 Jiří Zárevúcky
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Init binaries. These are actually baked into the bootloader.
rd_init = [
	# IMPORTANT: The order of entries is important for bootloader!
	'srv/ns',
	'srv/loader',
	'app/init',
	'srv/locsrv',
	'srv/bd/rd',
	'srv/vfs',
	'srv/logger',
	'srv/fs/' + RDFMT,
]

# Binaries allowed on the initrd image when CONFIG_BAREBONE is enabled.
rd_essential = [
	'app/bdsh',
	'app/getterm',
	'app/kio',

	'srv/devman',
	'srv/fs/locfs',
	'srv/hid/console',
	'srv/hid/input',
	'srv/hid/output',
	'srv/klog',
]

rd_essential_drv = [
	'drv/root/root',
	'drv/root/virt',
	'drv/fb/kfb',
]

rd_drv = rd_essential_drv

if CONFIG_FB
	rd_essential += [
		'app/launcher',
		'app/terminal',

		'srv/hid/display',
	]
endif
