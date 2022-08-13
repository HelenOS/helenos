#
# SPDX-FileCopyrightText: 2021 Jiri Svoboda
# SPDX-FileCopyrightText: 2019 Jiří Zárevúcky
#
# SPDX-License-Identifier: BSD-3-Clause
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


# TODO: Enable --gc-sections.
arch_kernel_link_args = [ '-nostdlib', '-Wl,--no-gc-sections' ]
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
	'drv/block/ata_bd',
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
	'drv/block/ata_bd',
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
