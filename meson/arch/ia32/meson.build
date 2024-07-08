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

_march = '-march=' + '-'.join(PROCESSOR.split('_'))
if PROCESSOR == 'core'
	# FIXME
	_march = '-march=prescott'
endif


arch_uspace_c_args = [
	_march,
	'-D__LE__',
	'-mno-tls-direct-seg-refs',
	'-fno-omit-frame-pointer',
]

arch_kernel_c_args = arch_uspace_c_args + [
	_march,
	'-mno-mmx',
	'-mno-sse',
	'-mno-sse2',
	'-mno-sse3',
	'-mno-3dnow',
]

arch_kernel_link_args = [ '-nostdlib' ]
arch_uspace_link_args = [ '-nostdlib', '-lgcc' ]

# Binaries to be included in a barebone build
rd_essential += [
	'app/edit',
	'app/mixerctl',
	'app/wavplay',

	'srv/audio/hound',
]

# Drivers to be included in a barebone build
rd_essential_drv += [
	'drv/intctl/apic',
	'drv/intctl/i8259',
	'drv/platform/pc',
	'drv/block/isa-ide',
	'drv/bus/pci/pciintel',
	'drv/bus/isa',
	'drv/audio/sb16',
	'drv/char/i8042',
	'drv/hid/ps2mouse',
	'drv/hid/xtkbd',
]

# Drivers to be included in a regular build
rd_drv += [
	'drv/audio/hdaudio',
	'drv/audio/sb16',
	'drv/block/ahci',
	'drv/block/isa-ide',
	'drv/block/pc-floppy',
	'drv/block/pci-ide',
	'drv/block/usbmast',
	'drv/block/virtio-blk',
	'drv/bus/isa',
	'drv/bus/pci/pciintel',
	'drv/bus/usb/ehci',
	'drv/bus/usb/ohci',
	'drv/bus/usb/uhci',
	'drv/bus/usb/usbdiag',
	'drv/bus/usb/usbflbk',
	'drv/bus/usb/usbhub',
	'drv/bus/usb/usbmid',
	'drv/bus/usb/vhc',
	'drv/bus/usb/xhci',
	'drv/char/i8042',
	'drv/char/ns8250',
	'drv/char/pc-lpt',
	'drv/hid/ps2mouse',
	'drv/hid/xtkbd',
	'drv/hid/usbhid',
	'drv/intctl/apic',
	'drv/intctl/i8259',
	'drv/nic/ar9271',
	'drv/nic/e1k',
	'drv/nic/ne2k',
	'drv/nic/rtl8139',
	'drv/nic/rtl8169',
	'drv/nic/virtio-net',
	'drv/platform/pc',
	'drv/time/cmos-rtc',
]
