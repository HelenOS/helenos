#
# SPDX-FileCopyrightText: 2019 Jiří Zárevúcky
#
# SPDX-License-Identifier: BSD-3-Clause
#

distdir = custom_target('distdir',
	output: 'distdir',
	input: [
		'build_dist.sh',
		boot_image,
		'ofboot.b',
		'bootinfo.txt',
		'yaboot',
		'yaboot.conf',
	],
	command: [ sh, '@INPUT0@', '@INPUT1@', meson.current_source_dir(), '@OUTPUT@' ],
)

image_iso = custom_target('image.iso',
	output: 'image.iso',
	input: [ distdir, 'maps' ],
	command: [
		genisoimage,
		'-hfs',
		'-part',
		'-map', '@INPUT1@',
		'-no-desktop',
		'-hfs-volid', 'HelenOS',
		'-hfs-bless', '@INPUT0@/boot',
		'-r',
		'-o', '@OUTPUT@',
		'@INPUT0@',
	],
)

POST_INPUT = image_iso
