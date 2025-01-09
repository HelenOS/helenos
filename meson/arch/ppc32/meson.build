#
# Copyright (c) 2023 Jiri Svoboda
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

_common_c_args = [ '-D__BE__', '-fno-omit-frame-pointer', '-m32', '-Wa,-a32', '-mcpu=powerpc' ]

arch_kernel_c_args = _common_c_args + [ '-msoft-float' ]
arch_uspace_c_args = _common_c_args + [ CONFIG_FPU ? '-mhard-float' : '-msoft-float' ]
arch_boot_c_args = arch_kernel_c_args
arch_kernel_link_args = [ '-nostdlib', '-Wl,-z,max-page-size=0x1000' ]
arch_uspace_link_args = [ '-nostdlib', '-lgcc', '-Wl,-z,max-page-size=0x1000' ]
arch_boot_link_args = ldflags_ignore_rwx_segments

rd_essential += [
	'app/ofw'
]

rd_essential_drv += [
	'drv/platform/mac',
	'drv/bus/adb/cuda_adb',
	'drv/bus/pci/pciintel',
	'drv/bus/usb/ohci',
	'drv/bus/usb/usbdiag',
	'drv/bus/usb/usbflbk',
	'drv/bus/usb/usbhub',
	'drv/bus/usb/usbmid',
	'drv/bus/usb/vhc',
	'drv/block/usbmast',
	'drv/hid/adb-kbd',
	'drv/hid/adb-mouse',
	'drv/hid/usbhid',
]

rd_drv += rd_essential_drv
