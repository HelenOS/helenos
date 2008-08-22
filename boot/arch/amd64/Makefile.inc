#
# Copyright (c) 2006 Martin Decky
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

INIT_TASKS = \
	$(USPACEDIR)/srv/ns/ns \
	$(USPACEDIR)/srv/loader/loader \
	$(USPACEDIR)/app/init/init \
	$(USPACEDIR)/srv/devmap/devmap \
	$(USPACEDIR)/srv/rd/rd \
	$(USPACEDIR)/srv/vfs/vfs
ifeq ($(RDFMT),tmpfs)
	INIT_TASKS += $(USPACEDIR)/srv/fs/tmpfs/tmpfs
endif
ifeq ($(RDFMT),fat)
	INIT_TASKS += $(USPACEDIR)/srv/fs/fat/fat
endif

RD_TASKS = \
	$(USPACEDIR)/srv/pci/pci \
	$(USPACEDIR)/srv/fb/fb \
	$(USPACEDIR)/srv/kbd/kbd \
	$(USPACEDIR)/srv/console/console \
	$(USPACEDIR)/app/tetris/tetris \
	$(USPACEDIR)/app/tester/tester \
	$(USPACEDIR)/app/klog/klog \
	$(USPACEDIR)/app/bdsh/bdsh \
	$(USPACEDIR)/srv/fs/tmpfs/tmpfs \
	$(USPACEDIR)/srv/fs/fat/fat

build: $(BASE)/image.iso

$(BASE)/image.iso: arch/$(ARCH)/grub/stage2_eltorito arch/$(ARCH)/grub/menu.lst $(KERNELDIR)/kernel.bin $(INIT_TASKS) $(RD_TASKS)
	mkdir -p arch/$(ARCH)/iso/boot/grub
	cp arch/$(ARCH)/grub/stage2_eltorito arch/$(ARCH)/iso/boot/grub/
ifneq ($(RDFMT),tmpfs)
	cat arch/$(ARCH)/grub/menu.lst | grep -v "tmpfs" >arch/$(ARCH)/iso/boot/grub/menu.lst
endif
ifneq ($(RDFMT),fat)
	cat arch/$(ARCH)/grub/menu.lst | grep -v "fat" >arch/$(ARCH)/iso/boot/grub/menu.lst
endif
	cp $(KERNELDIR)/kernel.bin arch/$(ARCH)/iso/boot/
	for task in $(INIT_TASKS) ; do \
		cp $$task arch/$(ARCH)/iso/boot/ ; \
	done
	for task in $(RD_TASKS) ; do \
		cp $$task $(USPACEDIR)/dist/sbin/ ; \
	done
ifeq ($(RDFMT),tmpfs)
	$(BASE)/tools/mktmpfs.py $(USPACEDIR)/dist/ arch/$(ARCH)/iso/boot/initrd.fs
endif
ifeq ($(RDFMT),fat)
	$(BASE)/tools/mkfat.sh $(USPACEDIR)/dist/ arch/$(ARCH)/iso/boot/initrd.fs
endif
	$(BASE)/tools/mkhord.py 4096 arch/$(ARCH)/iso/boot/initrd.fs arch/$(ARCH)/iso/boot/initrd.img
	rm arch/$(ARCH)/iso/boot/initrd.fs
	mkisofs -J -r -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 -boot-info-table -o $(BASE)/image.iso arch/$(ARCH)/iso/

clean:
	-for task in $(RD_TASKS) ; do \
		rm -f $(USPACEDIR)/dist/sbin/`basename $$task` ; \
	done
	-rm -fr arch/$(ARCH)/iso
	-rm -f $(BASE)/image.iso
