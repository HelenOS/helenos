#
# Copyright (c) 2005 Martin Decky
# Copyright (c) 2007 Jakub Jermar
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

NET_BASE = ../..
USPACE_PREFIX = ../../../..
ROOT_PATH = $(USPACE_PREFIX)/..

COMMON_MAKEFILE = $(ROOT_PATH)/Makefile.common
CONFIG_MAKEFILE = $(ROOT_PATH)/Makefile.config

BINARY = dp8390

-include $(COMMON_MAKEFILE)
-include $(CONFIG_MAKEFILE)

SOURCES = \
	dp8390.c \
	dp8390_module.c \
	ne2000.c \
	$(NET_BASE)/module.c \
	$(NET_BASE)/modules.c \
	$(NET_BASE)/netif/netif.c \
	$(NET_BASE)/structures/measured_strings.c \
	$(NET_BASE)/structures/packet/packet.c \
	$(NET_BASE)/structures/packet/packet_client.c \
	$(NET_BASE)/structures/packet/packet_remote.c

ifeq ($(CONFIG_NETWORKING),module)
	SOURCES += \
		$(NET_BASE)/checksum.c \
		$(NET_BASE)/nil/eth/eth.c \
		$(NET_BASE)/net/net_remote.c \
		$(NET_BASE)/netif/netif_nil_bundle.c
endif

ifeq ($(CONFIG_NETWORKING),modular)
	SOURCES += \
		$(NET_BASE)/nil/nil_remote.c \
		$(NET_BASE)/netif/netif_standalone.c
endif

include $(USPACE_PREFIX)/Makefile.common
