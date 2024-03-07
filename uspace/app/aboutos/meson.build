#
# Copyright (c) 2024 Jiri Svoboda
# Copyright (c) 2012 Petr Koupy
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

deps = [ 'gfximage', 'ui' ]

_images = files('gfx/helenos.tga')

_images_zip = custom_target('aboutos_images.zip',
	input : _images,
	output : [ 'images.zip' ],
	command : [ mkarray, '@OUTDIR@', 'images', 'image', 'image', uspace_as_prolog, '.data', '@INPUT@' ],
)
_imgs_s = custom_target('aboutos_images.s',
	input : _images_zip,
	output : [ 'images.s' ],
	command : [ unzip, '-p', '@INPUT@', 'images.s' ],
	capture : true,
)
_imgs_h = custom_target('aboutos_images.h',
	input : _images_zip,
	output : [ 'images.h' ],
	command : [ unzip, '-p', '@INPUT@', 'images.h' ],
	capture : true,
)
_imgs_desc_c = custom_target('aboutos_images_desc.c',
	input : _images_zip,
	output : [ 'images_desc.c' ],
	command : [ unzip, '-p', '@INPUT@', 'images_desc.c' ],
	capture : true,
)

c_args += [ '-DHELENOS_RELEASE=' + HELENOS_RELEASE, '-DHELENOS_COPYRIGHT=' + HELENOS_COPYRIGHT, '-DHELENOS_CODENAME=' + HELENOS_CODENAME ]
src = [ files('aboutos.c'), _imgs_s, _imgs_h, _imgs_desc_c ]
