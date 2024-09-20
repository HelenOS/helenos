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

# Init binaries. These are actually baked into the bootloader.
rd_init = [
	# ELF file with DWARF debug sections for kernel.
	# Kernel detects this entry by name.
	'kernel.dbg',

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
	'app/tester',

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
		'app/taskbar',
		'app/terminal',

		'srv/hid/display',
	]
endif
