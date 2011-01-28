#
# Copyright (c) 2011 Vojtech Horky
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

USPACE_PREFIX = ../..
# acronym for virtual USB hub
# (it is really annoying to write long names)
BINARY = vuh

LIBS = $(LIBUSB_PREFIX)/libusb.a $(LIBUSBVIRT_PREFIX)/libusbvirt.a
EXTRA_CFLAGS = -DSTANDALONE_HUB \
	-DHUB_PORT_COUNT=10 \
	-I$(LIBUSB_PREFIX)/include -I$(LIBUSBVIRT_PREFIX)/include -I$(LIBDRV_PREFIX)/include

SOURCES = \
	main.c \
	$(STOLEN_VHC_SOURCES)

STOLEN_VHC_SOURCES = \
	hub.c \
	virthub.c \
	virthubops.c 
STOLEN_VHC_HEADERS = \
	hub.h \
	virthub.h

PRE_DEPEND = $(STOLEN_VHC_SOURCES) $(STOLEN_VHC_HEADERS)

EXTRA_CLEAN = $(STOLEN_VHC_SOURCES) $(STOLEN_VHC_HEADERS)

HUB_IN_VHC = $(USPACE_PREFIX)/drv/vhc/hub

include $(USPACE_PREFIX)/Makefile.common


hub.h: $(HUB_IN_VHC)/hub.h
	ln -sfn $(HUB_IN_VHC)/$@ $@

hub.c: $(HUB_IN_VHC)/hub.c
	ln -sfn $(HUB_IN_VHC)/$@ $@

virthub.h: $(HUB_IN_VHC)/virthub.h
	ln -sfn $(HUB_IN_VHC)/$@ $@

virthub.c: $(HUB_IN_VHC)/virthub.c
	ln -sfn $(HUB_IN_VHC)/$@ $@

virthubops.c: $(HUB_IN_VHC)/virthubops.c
	ln -sfn $(HUB_IN_VHC)/$@ $@
