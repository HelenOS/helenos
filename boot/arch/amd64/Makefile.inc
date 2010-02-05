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

include Makefile.common

RD_SRVS += \
	$(USPACEDIR)/srv/dd/dd \
	$(USPACEDIR)/srv/bd/ata_bd/ata_bd \
	$(USPACEDIR)/srv/hw/char/i8042/i8042 \
#	$(USPACEDIR)/srv/hw/bus/pci/pci \
	$(USPACEDIR)/srv/hid/char_mouse/char_ms

MODULES := $(notdir $(COMPONENTS))

build: $(BASE)/image.iso

$(BASE)/image.iso: arch/$(BARCH)/grub/stage2_eltorito $(KERNELDIR)/kernel.bin $(INIT_TASKS) $(RD_SRVS) $(RD_APPS)
	mkdir -p $(TMP)/boot/grub
	cp arch/$(BARCH)/grub/stage2_eltorito $(TMP)/boot/grub/
	
	echo "default 0" > $(TMP)/boot/grub/menu.lst
	echo "timeout 10" >> $(TMP)/boot/grub/menu.lst
	echo "" >> $(TMP)/boot/grub/menu.lst
	echo "title=HelenOS" >> $(TMP)/boot/grub/menu.lst
	echo "	root (cd)" >> $(TMP)/boot/grub/menu.lst
	for module in $(MODULES) $(INITRD).img ; do \
		if [ $$module = kernel.bin ] ; then \
			echo "	kernel /boot/$$module" >> $(TMP)/boot/grub/menu.lst ; \
		else \
			echo "	module /boot/$$module" >> $(TMP)/boot/grub/menu.lst ; \
		fi \
	done
	
	cp $(KERNELDIR)/kernel.bin $(TMP)/boot/
	for task in $(INIT_TASKS) ; do \
		cp $$task $(TMP)/boot/ ; \
	done
	for file in $(RD_SRVS) ; do \
		cp $$file $(USPACEDIR)/dist/srv/ ; \
	done
	for file in $(RD_APPS) ; do \
		cp $$file $(USPACEDIR)/dist/app/ ; \
	done
	
ifeq ($(RDFMT),tmpfs)
	$(BASE)/tools/mktmpfs.py $(USPACEDIR)/dist/ $(TMP)/boot/$(INITRD).fs
endif
ifeq ($(RDFMT),fat)
	$(BASE)/tools/mkfat.py $(USPACEDIR)/dist/ $(TMP)/boot/$(INITRD).fs
endif
	$(BASE)/tools/mkhord.py 4096 $(TMP)/boot/$(INITRD).fs $(TMP)/boot/$(INITRD).img
	rm $(TMP)/boot/$(INITRD).fs
	mkisofs -J -r -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 -boot-info-table -o $@ $(TMP)/

clean:
	for file in $(RD_SRVS) ; do \
		rm -f $(USPACEDIR)/dist/srv/`basename $$file` ; \
	done
	for file in $(RD_APPS) ; do \
		rm -f $(USPACEDIR)/dist/app/`basename $$file` ; \
	done
	rm -fr $(TMP)
	rm -f $(BASE)/image.iso
