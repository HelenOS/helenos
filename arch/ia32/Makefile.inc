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

build: image.boot

image.boot: kernel uspace
	gunzip -c arch/$(ARCH)/grub/grub.img.gz > image.boot
	e2cp arch/$(ARCH)/grub/grub.conf image.boot:/boot/grub/grub.conf
	e2cp $(KERNELDIR)/kernel.bin image.boot:/boot/kernel.bin
	e2cp $(USPACEDIR)/init/init image.boot:/init
	e2cp $(USPACEDIR)/ns/ns image.boot:/ns
	e2cp $(USPACEDIR)/pci/pci image.boot:/pci
	e2cp $(USPACEDIR)/fb/fb image.boot:/fb
	e2cp $(USPACEDIR)/kbd/kbd image.boot:/kbd

clean: clean_kernel clean_uspace
	-rm -f image.boot

arch_distclean: distclean_kernel distclean_uspace
