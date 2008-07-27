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

## Include configuration
#

-include Makefile.config

## Setup platform configuration
#

ifeq ($(PLATFORM),amd64)
	KARCH = amd64
	MACHINE = opteron
	UARCH = amd64
	BARCH = amd64
endif

ifeq ($(PLATFORM),arm32)
	KARCH = arm32
	UARCH = arm32
	BARCH = arm32
endif

ifeq ($(PLATFORM),ia32)
	KARCH = ia32
	UARCH = ia32
	BARCH = ia32
endif

ifeq ($(PLATFORM),ia64)
	KARCH = ia64
	UARCH = ia64
	BARCH = ia64
endif

ifeq ($(PLATFORM),mips32)
	KARCH = mips32
	BARCH = mips32
	
	ifeq ($(MACHINE),msim)
		UARCH = mips32
		IMAGE = binary
	endif
	
	ifeq ($(MACHINE),simics)
		UARCH = mips32
		IMAGE = ecoff
	endif
	
	ifeq ($(MACHINE),bgxemul)
		UARCH = mips32eb
		IMAGE = ecoff
	endif
	
	ifeq ($(MACHINE),lgxemul)
		UARCH = mips32
		IMAGE = ecoff
	endif
endif

ifeq ($(PLATFORM),ppc32)
	KARCH = ppc32
	UARCH = ppc32
	BARCH = ppc32
endif

ifeq ($(PLATFORM),ppc64)
	KARCH = ppc64
	UARCH = ppc64
	BARCH = ppc64
endif

ifeq ($(PLATFORM),sparc64)
	KARCH = sparc64
	UARCH = sparc64
	BARCH = sparc64
endif

ifeq ($(PLATFORM),ia32xen)
	KARCH = ia32xen
	UARCH = ia32
	BARCH = ia32xen
endif

.PHONY: all build config distclean clean cscope

all:
	tools/config.py HelenOS.config default $(PLATFORM) $(COMPILER) $(CONFIG_DEBUG)
	$(MAKE) -C . build

build:
ifneq ($(MACHINE),)
	$(MAKE) -C kernel ARCH=$(KARCH) COMPILER=$(COMPILER) CONFIG_DEBUG=$(CONFIG_DEBUG) MACHINE=$(MACHINE)
else
	$(MAKE) -C kernel ARCH=$(KARCH) COMPILER=$(COMPILER) CONFIG_DEBUG=$(CONFIG_DEBUG)
endif
	$(MAKE) -C uspace ARCH=$(UARCH) COMPILER=$(COMPILER) CONFIG_DEBUG=$(CONFIG_DEBUG)
ifneq ($(IMAGE),)
	$(MAKE) -C boot ARCH=$(BARCH) COMPILER=$(COMPILER) CONFIG_DEBUG=$(CONFIG_DEBUG) IMAGE=$(IMAGE)
else
	$(MAKE) -C boot ARCH=$(BARCH) COMPILER=$(COMPILER) CONFIG_DEBUG=$(CONFIG_DEBUG)
endif

config:
	tools/config.py HelenOS.config

distclean:
	-$(MAKE) -C kernel distclean
	-$(MAKE) -C uspace distclean
	-$(MAKE) -C boot distclean
	-rm Makefile.config

clean:
	-$(MAKE) -C kernel clean
	-$(MAKE) -C uspace clean
	-$(MAKE) -C boot clean

cscope:
	-rm cscope.out
	-find kernel boot uspace -regex '^.*\.[chsS]$$' -print >srclist
	-cscope -bi srclist
