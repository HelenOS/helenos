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

USPACE_PREFIX = ../../..
LIBS = $(LIBFB_PREFIX)/libfb.a
EXTRA_CFLAGS += -I$(LIBFB_PREFIX)
BINARY = console

SOURCES = \
	console.c \
	images.c

IMAGES = \
	gfx/helenos.tga \
	gfx/nameic.tga \
	gfx/cons_data.tga \
	gfx/cons_dis.tga \
	gfx/cons_dis_sel.tga \
	gfx/cons_idle.tga \
	gfx/cons_sel.tga \
	gfx/cons_kernel.tga \
	gfx/anim_1.tga \
	gfx/anim_2.tga \
	gfx/anim_3.tga \
	gfx/anim_4.tga

PRE_DEPEND = images.c images.h
EXTRA_CLEAN = images.c images.h

include $(USPACE_PREFIX)/Makefile.common

images.c images.h: $(IMAGES)
	$(ROOT_PATH)/tools/mkarray.py images CONSOLE_IMAGES $^
