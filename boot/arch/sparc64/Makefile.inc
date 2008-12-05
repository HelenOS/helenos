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

TMP = distroot

ifeq ($(CONFIG_AOUT_ISOFS_B),n)
	SILO_PACKAGE=silo.patched.tar.gz
endif

ifeq ($(CONFIG_AOUT_ISOFS_B),y)
	SILO_PACKAGE=silo.tar.gz
endif

build: $(BASE)/image.iso

ifeq ($(CONFIG_RD_EXTERNAL),y)
SILO_CONF_FILTER = cat
else
SILO_CONF_FILTER = grep -v initrd
endif

$(BASE)/image.iso: depend arch/$(ARCH)/loader/image.boot
	mkdir -p $(TMP)/boot
	mkdir -p $(TMP)/HelenOS
	cat arch/$(ARCH)/silo/$(SILO_PACKAGE) | (cd $(TMP)/boot; tar xvfz -)
	cp arch/$(ARCH)/silo/README arch/$(ARCH)/silo/COPYING $(TMP)/boot
	cat arch/$(ARCH)/silo/silo.conf | $(SILO_CONF_FILTER) >$(TMP)/boot/silo.conf
	cp arch/$(ARCH)/loader/image.boot $(TMP)/HelenOS/image.boot
	gzip -f $(TMP)/HelenOS/image.boot
ifeq ($(CONFIG_RD_EXTERNAL),y)
	cp arch/$(ARCH)/loader/initrd.img $(TMP)/HelenOS/initrd.img
endif
	mkisofs -f -G $(TMP)/boot/isofs.b -B ... -r -o $(BASE)/image.iso $(TMP)/

depend:
	-rm arch/$(ARCH)/loader/image.boot

arch/$(ARCH)/loader/image.boot:
	$(MAKE) -C arch/$(ARCH)/loader COMPILER=$(COMPILER) KERNELDIR=../../../$(KERNELDIR) USPACEDIR=../../../$(USPACEDIR) "DEFS=$(DEFS)"

clean: generic_clean
	$(MAKE) -C arch/$(ARCH)/loader clean COMPILER=$(COMPILER) KERNELDIR=../../../$(KERNELDIR) USPACEDIR=../../../$(USPACEDIR)
	-rm -fr $(TMP)
	-rm -f $(BASE)/image.iso
