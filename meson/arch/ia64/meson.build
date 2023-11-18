#
# Copyright (c) 2021 Jiri Svoboda
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

arch_uspace_c_args = [ '-D__LE__', '-fno-unwind-tables' ]
arch_kernel_c_args = arch_uspace_c_args + [ '-mconstant-gp', '-mfixed-range=f32-f127' ]
if MACHINE == 'i460GX'
	arch_kernel_c_args += [ '-DI460GX' ]
endif

arch_boot_c_args = [ '-D__LE__', '-fno-unwind-tables', '-mfixed-range=f32-f127', '-mno-pic', '-mno-sdata' ]

# work around GCC bug 112604
arch_uspace_c_args += ['-fno-unswitch-loops']

arch_uspace_link_args = [ '-nostdlib', '-lgcc' ]
arch_kernel_link_args = [ '-nostdlib', '-Wl,-EL', '-Wl,--no-warn-rwx-segments']
arch_boot_link_args = ['-Wl,--no-warn-rwx-segments']


if MACHINE == 'i460GX'
	rd_essential_drv += [
		'drv/platform/pc',
		'drv/bus/pci/pciintel',
		'drv/bus/isa',
		'drv/char/i8042',
		'drv/hid/xtkbd',
	]

elif MACHINE == 'ski'
	rd_essential_drv += [
		'drv/char/ski-con',
		'drv/platform/ski',
	]

endif

rd_drv += rd_essential_drv
