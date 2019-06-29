distdir = custom_target('distdir',
	output: 'distdir',
	input: [
		'build_dist.sh',
		boot_image,
		'a.out/isofs.b',
		'a.out/second.b',
		'silo.conf'

	],
	command: [ sh, '@INPUT0@', '@INPUT1@', meson.current_source_dir(), '@OUTPUT@' ],
)

image_iso = custom_target('image.iso',
	output: 'image.iso',
	input: [ distdir, 'a.out/isofs.b' ],
	command: [
		genisoimage,
		'-f',
		'-G', '@INPUT1@',
		'-B', '...',
		'-r',
		'-o', '@OUTPUT@',
		'@INPUT0@',
	],
)

POST_INPUT = image_iso
