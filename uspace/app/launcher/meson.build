#
# SPDX-FileCopyrightText: 2012 Petr Koupy
#
# SPDX-License-Identifier: BSD-3-Clause
#

deps = [ 'gfximage', 'ui' ]

_images = files('gfx/helenos.tga')

_images_zip = custom_target('launcher_images.zip',
	input : _images,
	output : [ 'images.zip' ],
	command : [ mkarray, '@OUTDIR@', 'images', 'image', uspace_as_prolog, '.data', '@INPUT@' ],
)
_imgs_s = custom_target('launcher_images.s',
	input : _images_zip,
	output : [ 'images.s' ],
	command : [ unzip, '-p', '@INPUT@', 'images.s' ],
	capture : true,
)
_imgs_h = custom_target('launcher_images.h',
	input : _images_zip,
	output : [ 'images.h' ],
	command : [ unzip, '-p', '@INPUT@', 'images.h' ],
	capture : true,
)
_imgs_desc_c = custom_target('launcher_images_desc.c',
	input : _images_zip,
	output : [ 'images_desc.c' ],
	command : [ unzip, '-p', '@INPUT@', 'images_desc.c' ],
	capture : true,
)

src = [ files('launcher.c'), _imgs_s, _imgs_h, _imgs_desc_c ]
