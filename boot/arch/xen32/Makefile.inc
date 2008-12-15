#
# Copyright (C) 2006 Martin Decky
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

build: image.iso

image.iso: kernel uspace
	mkdir -p arch/$(ARCH)/iso/boot/grub
	cp arch/$(ARCH)/grub/stage2_eltorito arch/$(ARCH)/iso/boot/grub/
	cp arch/$(ARCH)/grub/menu.lst arch/$(ARCH)/iso/boot/grub/
	cp arch/$(ARCH)/grub/xen.gz arch/$(ARCH)/iso/boot/
	cp $(KERNELDIR)/kernel.bin arch/$(ARCH)/iso/boot/
	cp $(USPACEDIR)/init/init arch/$(ARCH)/iso/boot/
	cp $(USPACEDIR)/ns/ns arch/$(ARCH)/iso/boot/
	cp $(USPACEDIR)/pci/pci arch/$(ARCH)/iso/boot/
	cp $(USPACEDIR)/fb/fb arch/$(ARCH)/iso/boot/
	cp $(USPACEDIR)/kbd/kbd arch/$(ARCH)/iso/boot/
	cp $(USPACEDIR)/console/console arch/$(ARCH)/iso/boot/
	cp $(USPACEDIR)/tetris/tetris arch/$(ARCH)/iso/boot/
	cp $(USPACEDIR)/ipcc/ipcc arch/$(ARCH)/iso/boot/
	cp $(USPACEDIR)/klog/klog arch/$(ARCH)/iso/boot/
	mkisofs -J -r -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 -boot-info-table -o image.iso arch/$(ARCH)/iso/

clean: clean_kernel clean_uspace
	-rm -fr arch/$(ARCH)/iso
	-rm -f image.iso

arch_distclean: distclean_kernel distclean_uspace
