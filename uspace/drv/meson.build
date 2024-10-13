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

drvs = [
	'audio/hdaudio',
	'audio/sb16',
	'root/root',
	'root/virt',
	'block/ahci',
	'block/ddisk',
	'block/isa-ide',
	'block/pc-floppy',
	'block/pci-ide',
	'block/usbmast',
	'block/virtio-blk',
	'bus/adb/cuda_adb',
	'bus/isa',
	'bus/pci/pciintel',
	'bus/usb/ehci',
	'bus/usb/ohci',
	'bus/usb/uhci',
	'bus/usb/usbdiag',
	'bus/usb/usbflbk',
	'bus/usb/usbhub',
	'bus/usb/usbmid',
	'bus/usb/vhc',
	'bus/usb/xhci',
	'char/i8042',
	'char/msim-con',
	'char/ns8250',
	'char/pc-lpt',
	'char/pl011',
	'char/pl050',
	'char/ski-con',
	'char/sun4v-con',
	'fb/amdm37x_dispc',
	'fb/kfb',
	'hid/adb-kbd',
	'hid/adb-mouse',
	'hid/atkbd',
	'hid/ps2mouse',
	'hid/usbhid',
	'hid/xtkbd',
	'intctl/apic',
	'intctl/gicv2',
	'intctl/i8259',
	'intctl/icp-ic',
	'intctl/obio',
	'nic/ne2k',
	'nic/e1k',
	'nic/rtl8139',
	'nic/rtl8169',
	'nic/ar9271',
	'nic/virtio-net',
	'platform/amdm37x',
	'platform/arm64virt',
	'platform/hikey960',
	'platform/icp',
	'platform/mac',
	'platform/malta',
	'platform/msim',
	'platform/pc',
	'platform/ski',
	'platform/sun4v',
	'test/test1',
	'test/test2',
	'test/test3',
	'time/cmos-rtc',
]

# sun4u driver won't compile on 32-bit targets
if UARCH == 'sparc64'
	drvs += 'platform/sun4u'
endif
