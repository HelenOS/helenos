#
# Copyright (c) 2019 Jiří Zárevúcky
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
