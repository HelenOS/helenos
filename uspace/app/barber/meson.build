#
# Copyright (c) 2014 Martin Decky
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

deps = [ 'ui', 'gfximage', 'compress', 'device' ]

_images = files(
	'gfx/frame01.tga.gz',
	'gfx/frame02.tga.gz',
	'gfx/frame03.tga.gz',
	'gfx/frame04.tga.gz',
	'gfx/frame05.tga.gz',
	'gfx/frame06.tga.gz',
	'gfx/frame07.tga.gz',
	'gfx/frame08.tga.gz',
	'gfx/frame09.tga.gz',
	'gfx/frame10.tga.gz',
	'gfx/frame11.tga.gz',
	'gfx/frame12.tga.gz',
	'gfx/frame13.tga.gz',
	'gfx/frame14.tga.gz',
	'gfx/frame15.tga.gz',
	'gfx/frame16.tga.gz',
	'gfx/frame17.tga.gz',
	'gfx/frame18.tga.gz',
	'gfx/frame19.tga.gz',
	'gfx/frame20.tga.gz',
	'gfx/frame21.tga.gz',
	'gfx/frame22.tga.gz',
	'gfx/frame23.tga.gz',
	'gfx/frame24.tga.gz',
	'gfx/frame25.tga.gz',
	'gfx/frame26.tga.gz',
	'gfx/frame27.tga.gz',
	'gfx/frame28.tga.gz',
	'gfx/frame29.tga.gz',
	'gfx/frame30.tga.gz',
)

_images_tiny = files(
	'gfx-tiny/frame01t.tga.gz',
	'gfx-tiny/frame02t.tga.gz',
	'gfx-tiny/frame03t.tga.gz',
	'gfx-tiny/frame04t.tga.gz',
	'gfx-tiny/frame05t.tga.gz',
	'gfx-tiny/frame06t.tga.gz',
	'gfx-tiny/frame07t.tga.gz',
	'gfx-tiny/frame08t.tga.gz',
	'gfx-tiny/frame09t.tga.gz',
	'gfx-tiny/frame10t.tga.gz',
	'gfx-tiny/frame11t.tga.gz',
	'gfx-tiny/frame12t.tga.gz',
	'gfx-tiny/frame13t.tga.gz',
	'gfx-tiny/frame14t.tga.gz',
	'gfx-tiny/frame15t.tga.gz',
	'gfx-tiny/frame16t.tga.gz',
	'gfx-tiny/frame17t.tga.gz',
	'gfx-tiny/frame18t.tga.gz',
	'gfx-tiny/frame19t.tga.gz',
	'gfx-tiny/frame20t.tga.gz',
	'gfx-tiny/frame21t.tga.gz',
	'gfx-tiny/frame22t.tga.gz',
	'gfx-tiny/frame23t.tga.gz',
	'gfx-tiny/frame24t.tga.gz',
	'gfx-tiny/frame25t.tga.gz',
	'gfx-tiny/frame26t.tga.gz',
	'gfx-tiny/frame27t.tga.gz',
	'gfx-tiny/frame28t.tga.gz',
	'gfx-tiny/frame29t.tga.gz',
	'gfx-tiny/frame30t.tga.gz',
)

_tarball = custom_target('barber_images.tar',
	input: _images,
	output: 'barber_images.tar',
	command: tar,
)

# TODO

# Normal images

_images_zip = custom_target('barber_images.zip',
	input : _images,
	output : [ 'images.zip' ],
	command : [ mkarray, '@OUTDIR@', 'images', 'image', 'image', uspace_as_prolog, '.data', '@INPUT@' ],
)
_imgs_s = custom_target('barber_images.s',
	input : _images_zip,
	output : [ 'images.s' ],
	command : [ unzip, '-p', '@INPUT@', 'images.s' ],
	capture : true,
)
_imgs_h = custom_target('barber_images.h',
	input : _images_zip,
	output : [ 'images.h' ],
	command : [ unzip, '-p', '@INPUT@', 'images.h' ],
	capture : true,
)
_imgs_desc_c = custom_target('barber_images_desc.c',
	input : _images_zip,
	output : [ 'images_desc.c' ],
	command : [ unzip, '-p', '@INPUT@', 'images_desc.c' ],
	capture : true,
)

# Tiny images

_images_tiny_zip = custom_target('barber_images_tiny.zip',
	input : _images_tiny,
	output : [ 'images_tiny.zip' ],
	command : [ mkarray, '@OUTDIR@', 'images_tiny', 'image', 'image_tiny', uspace_as_prolog, '.data', '@INPUT@' ],
)
_imgs_tiny_s = custom_target('barber_images_tiny.s',
	input : _images_tiny_zip,
	output : [ 'images_tiny.s' ],
	command : [ unzip, '-p', '@INPUT@', 'images_tiny.s' ],
	capture : true,
)
_imgs_tiny_h = custom_target('barber_images_tiny.h',
	input : _images_tiny_zip,
	output : [ 'images_tiny.h' ],
	command : [ unzip, '-p', '@INPUT@', 'images_tiny.h' ],
	capture : true,
)
_imgs_tiny_desc_c = custom_target('barber_images_tiny_desc.c',
	input : _images_tiny_zip,
	output : [ 'images_tiny_desc.c' ],
	command : [ unzip, '-p', '@INPUT@', 'images_tiny_desc.c' ],
	capture : true,
)

src = [ files('barber.c'), _imgs_s, _imgs_h, _imgs_desc_c,
    _imgs_tiny_s, _imgs_tiny_h, _imgs_tiny_desc_c ]
