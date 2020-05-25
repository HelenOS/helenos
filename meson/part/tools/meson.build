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

# Find all the programs and tools used by meson.
# Whenever you use an external command anywhere,
# first find and assign it to a variable here. Don't use its name directly.
# This way it's obvious what programs are used, and for some we can use
# the same argument list everywhere.

_tools_dir = meson.source_root() / 'tools'

basename = find_program('basename')
cc = meson.get_compiler('c')
config_py = find_program(_tools_dir / 'config.py')
cp = find_program('cp')
dirname = find_program('dirname')
doxygen = find_program('doxygen', required: false)
find = find_program('find')
grep = find_program('grep')
mkarray = find_program(_tools_dir / 'mkarray_for_meson.sh')
mkext4 = find_program(_tools_dir / 'mkext4.py')
mkfat = find_program(_tools_dir / 'mkfat.py')
mkuimage = find_program(_tools_dir / 'mkuimage.py')
objcopy = find_program('objcopy')
objdump = find_program('objdump')
patch = find_program('patch')
sed = find_program('sed')
unzip = find_program('unzip')
which = find_program('which')
cpc = find_program(_tools_dir / 'cc.sh')
cppcheck = find_program('cppcheck', required: false)

sh = [ find_program('sh'), '-u', '-e' ]
if debug_shell_scripts
	sh += '-x'
endif

genisoimage = find_program('genisoimage', required: false)
if genisoimage.found()
	genisoimage_type = 'genisoimage'
else
	genisoimage = find_program('mkisofs', required: false)
	if genisoimage.found()
		genisoimage_type = 'mkisofs'
	else
		xorriso = find_program('xorriso', required: false)
		if xorriso.found()
			genisoimage = [ xorriso, '-as', 'genisoimage' ]
			genisoimage_type = 'genisoimage'
		else
			error('Need genisoimage, mkisofs or xorriso.')
		endif
	endif
endif


autocheck = generator(find_program(_tools_dir / 'autocheck.awk'),
	arguments: [ '@INPUT@' ],
	output: '@PLAINNAME@.check.c',
	capture: true,
)

# TODO: bug in Meson
#gzip = generator(find_program('gzip'),
#	arguments: [ '--no-name', '-9', '--stdout', '@INPUT@' ],
#	output: '@PLAINNAME@.gz',
#	capture: true,
#)

gzip = [ find_program('gzip'), '--no-name', '-9', '--stdout', '@INPUT@' ]

# Tar's arguments make sure that the archive is reproducible.
tar = [
	find_program('tar'),
	'-c',
	'-f', '@OUTPUT@',
	'--mtime=1970-01-01 00:00:00Z',
	'--group=0',
	'--owner=0',
	'--numeric-owner',
	'--mode=go=rX,u+rw,a-s',
	'--no-acls',
	'--no-selinux',
	'--no-xattrs',
	'--format=ustar',
	'--transform', 's:@OUTDIR@/::',
	'@INPUT@',
]
