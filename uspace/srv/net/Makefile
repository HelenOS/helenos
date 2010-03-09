#
# Copyright (c) 2009 Lukas Mejdrech
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

include ../../../Makefile.config

# keep netif drivers before nil modules
# in order to support networking architectures build

ifeq ($(CONFIG_NETIF_DP8390),y)
	DIRS = netif/dp8390
else
	DIRS =
endif

DIRS += \
	netif/lo \
	nil/eth \
	nil/nildummy \
	net \
	net/start \
	socket \
	app/echo \
	app/ping \
	app/nettest1 \
	app/nettest2

DIRS_MODULAR = \
		il/arp \
		il/ip \
		tl/icmp \
		tl/udp \
		tl/tcp \

DIRS_ALL = $(DIRS) $(DIRS_MODULAR)

ifeq ($(NETWORKING), module)
	BUILDS := $(addsuffix .build,$(DIRS))
else
ifeq ($(NETWORKING), modular)
	BUILDS := $(addsuffix .build,$(DIRS_ALL))
endif
endif

CLEANS := $(addsuffix .clean,$(DIRS_ALL))

CHECKS := $(addsuffix .check,$(basename $(BUILDS)))

.PHONY: all build $(BUILDS) $(CLEANS) clean

all: $(BUILDS)

build: $(BUILDS)

check: $(CHECKS)

clean: $(CLEANS)
	find $(DIRS_ALL) ./ -name '*.o' -follow -exec rm \{\} \;

$(CHECKS):
	-$(MAKE) -C $(basename $@) check

$(CLEANS):
	-$(MAKE) -C $(basename $@) clean

$(BUILDS):
	$(MAKE) -C $(basename $@) all
