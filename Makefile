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

## Include configuration
#

-include Makefile.config

## Common flags
#

BASE = ..
KERNELDIR = $(BASE)/kernel
USPACEDIR = $(BASE)/uspace

## Setup arch configuration
#

-include arch/$(ARCH)/Makefile.inc

.PHONY: all build config distclean arch_distclean clean kernel uspace clean_kernel clean_uspace distclean_kernel distclean_uspace

all:
	tools/config.py default
	$(MAKE) -C . build $(ARCH)

config:
	tools/config.py

distclean: clean arch_distclean
	-rm Makefile.config

kernel:
	$(MAKE) -C $(KERNELDIR) NARCH=$(ARCH)

uspace:
	$(MAKE) -C $(USPACEDIR) NARCH=$(ARCH)

clean_kernel:
	$(MAKE) -C $(KERNELDIR) clean ARCH=$(ARCH)

clean_uspace:
	$(MAKE) -C $(USPACEDIR) clean ARCH=$(ARCH)

distclean_kernel:
	$(MAKE) -C $(KERNELDIR) distclean ARCH=$(ARCH)

distclean_uspace:
	$(MAKE) -C $(USPACEDIR) distclean ARCH=$(ARCH)
