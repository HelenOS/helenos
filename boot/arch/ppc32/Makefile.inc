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

build: $(BASE)/image.iso

$(BASE)/image.iso: arch/$(BARCH)/loader/image.boot
	mkdir -p $(TMP)/boot
	mkdir -p $(TMP)/ppc
	cp arch/$(BARCH)/loader/image.boot $(TMP)/boot/image.boot
	cp arch/$(BARCH)/yaboot/ofboot.b $(TMP)/boot/ofboot.b
	cp arch/$(BARCH)/yaboot/bootinfo.txt $(TMP)/ppc/bootinfo.txt
	cp arch/$(BARCH)/yaboot/yaboot $(TMP)/boot/yaboot
	cp arch/$(BARCH)/yaboot/yaboot.conf $(TMP)/boot/yaboot.conf
	mkisofs -hfs -part -map arch/$(BARCH)/yaboot/maps -no-desktop -hfs-volid "HelenOS" -hfs-bless $(TMP)/boot -r -o $@ $(TMP)/

arch/$(BARCH)/loader/image.boot: $(COMPONENTS) $(INIT_TASKS) $(RD_SRVS) $(RD_APPS) $(CFG)
	$(MAKE) -C arch/$(BARCH)/loader PRECHECK=$(PRECHECK)

clean: generic_clean
	$(MAKE) -C arch/$(BARCH)/loader clean
	rm -fr $(TMP)
	rm -f $(BASE)/image.iso
