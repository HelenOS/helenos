# Copyright (c) 2005 Martin Decky
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

## Accepted configuration directives
#

_src = []
_check = []

if CONFIG_ACPI
	_src += [ 'acpi/acpi.c', 'acpi/madt.c' ]
endif

if CONFIG_PAGE_PT
	_src += [ 'mm/page_pt.c', 'mm/as_pt.c' ]
endif

if CONFIG_PAGE_HT
	_src += [ 'mm/page_ht.c', 'mm/as_ht.c' ]
endif

if CONFIG_ASID
	_src += [ 'mm/asid.c' ]
endif

if CONFIG_ASID_FIFO
	_src += [ 'mm/asid_fifo.c' ]
endif

if CONFIG_SOFTINT
	_src += [ 'softint/division.c', 'softint/multiplication.c' ]
endif

if CONFIG_FB
	_src += [ 'fb/font-8x16.c', 'fb/fb.c', 'fb/bfb.c' ]
endif

if CONFIG_DSRLNIN
	_src += [ 'drivers/dsrln/dsrlnin.c' ]
endif

if CONFIG_DSRLNOUT
	_src += [ 'drivers/dsrln/dsrlnout.c' ]
endif

if CONFIG_I8042
	_src += [ 'drivers/i8042/i8042.c' ]
endif

if CONFIG_I8259
	_src += [ 'drivers/i8259/i8259.c' ]
endif

if CONFIG_NS16550
	_src += [ 'drivers/ns16550/ns16550.c' ]
endif

if CONFIG_PL011_UART
	_src += [ 'drivers/pl011/pl011.c' ]
endif

if CONFIG_PL050
	_src += [ 'drivers/pl050/pl050.c' ]
endif

if CONFIG_S3C24XX_IRQC
	_src += [ 'drivers/s3c24xx/irqc.c' ]
endif

if CONFIG_S3C24XX_UART
	_src += [ 'drivers/s3c24xx/uart.c' ]
endif

if CONFIG_OMAP_UART
	_src += [ 'drivers/omap/uart.c' ]
endif

if CONFIG_AM335X_TIMERS
	_src += [ 'drivers/am335x/timer.c' ]
endif

if CONFIG_BCM2835_MAILBOX
	_src += [ 'drivers/bcm2835/irc.c', 'drivers/bcm2835/mbox.c', 'drivers/bcm2835/timer.c' ]
endif

if CONFIG_GICV2
	_src += [ 'drivers/gicv2/gicv2.c' ]
endif

if CONFIG_VIA_CUDA
	_src += [ 'drivers/via-cuda/cuda.c' ]
endif

if CONFIG_PC_KBD
	_src += [ 'kbrd/kbrd.c', 'kbrd/scanc_pc.c' ]
endif

if CONFIG_AT_KBD
	_src += [ 'kbrd/kbrd_at.c', 'kbrd/scanc_at.c' ]
endif

if CONFIG_SUN_KBD
	_src += [ 'kbrd/kbrd.c', 'kbrd/scanc_sun.c' ]
endif

if CONFIG_MAC_KBD
	_src += [ 'kbrd/kbrd.c', 'kbrd/scanc_mac.c' ]
endif

if CONFIG_SRLN
	_src += [ 'srln/srln.c' ]
endif

if CONFIG_OFW_TREE
	_src += [ 'ofw/ofw_tree.c' ]
endif

if CONFIG_OFW_PCI
	_src += [ 'ofw/ebus.c', 'ofw/pci.c', 'ofw/sbus.c', 'ofw/upa.c' ]
endif

if CONFIG_MULTIBOOT
	_src += [ 'multiboot/common.c', 'multiboot/multiboot.c', 'multiboot/multiboot2.c' ]
	_check += [ 'multiboot/multiboot_memmap_struct.h', 'multiboot/multiboot_info_struct.h' ]
endif

if CONFIG_EGA
	_src += [ 'drivers/ega/ega.c' ]
endif

if CONFIG_IOMAP_BITMAP
	_src += [ 'ddi/ddi-bitmap.c' ]
endif

if CONFIG_IOMAP_DUMMY
	_src += [ 'ddi/ddi-dummy.c' ]
endif


genarch_src = []

foreach f : _src
	genarch_src += files('src' / f)
endforeach

foreach f : _check
	genarch_src += [ autocheck.process('include' / 'genarch' / f) ]
endforeach
