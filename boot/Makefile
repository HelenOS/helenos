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

-include ../version
-include Makefile.config

## Paths
#

BASE = ..
KERNELDIR = $(BASE)/kernel
USPACEDIR = $(BASE)/uspace

ifeq ($(CONFIG_DEBUG),y)
	DEFS += -DCONFIG_DEBUG
endif

ifeq ($(CONFIG_BAT),y)
	DEFS += -DCONFIG_BAT
endif

ifeq ($(CONFIG_SMP),y)
	DEFS += -DCONFIG_SMP
endif

.PHONY: all build config distclean clean generic_clean

all:
	../tools/config.py boot.config default $(ARCH) $(COMPILER) $(CONFIG_DEBUG) $(IMAGE)
	$(MAKE) -C . build

-include arch/$(ARCH)/Makefile.inc

config:
	../tools/config.py boot.config

distclean: clean
	-rm Makefile.config

generic_clean:
	-rm generic/*.o genarch/*.o
