#
# Copyright (c) 2006 Martin Decky
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

BUILD = true
POSTBUILD = 'yaboot'
POST_OUTPUT = 'image.iso'
boot_image_format = 'elf'

boot_src = files(
	'src/asm.S',
	'src/main.c',
	'src/ofw.c',

	'../../../common/stdc/mem.c',
	'../../genarch/src/ofw.c',
	'../../genarch/src/ofw_tree.c',
	'../../genarch/src/division.c',
	'../../genarch/src/multiplication.c',
	'../../generic/src/balloc.c',
	'../../generic/src/printf_core.c',
	'../../generic/src/vprintf.c',
	'../../generic/src/printf.c',
	'../../../common/str.c',
	'../../generic/src/version.c',
	'../../generic/src/inflate.c',
	'../../generic/src/gzip.c',
	'../../generic/src/tar.c',
	'../../generic/src/kernel.c',
	'../../generic/src/payload.c',
)
