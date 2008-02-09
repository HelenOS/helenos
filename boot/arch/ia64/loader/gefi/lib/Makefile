#
#  Copyright (C) 1999-2001 Hewlett-Packard Co.
#	Contributed by David Mosberger <davidm@hpl.hp.com>
#	Contributed by Stephane Eranian <eranian@hpl.hp.com>
#
# This file is part of the gnu-efi package.
#
#  GNU-EFI is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2, or (at your option)
#  any later version.
#
#  GNU-EFI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with GNU-EFI; see the file COPYING.  If not, write to the Free
#  Software Foundation, 59 Temple Place - Suite 330, Boston, MA
#  02111-1307, USA.
#

include ../Make.defaults
CDIR = $(TOPDIR)/..
FILES = boxdraw smbios console crc data debug dpath  \
        error event guid hand hw init lock   \
        misc print sread str \
	runtime/rtlock runtime/efirtlib runtime/rtstr runtime/vm runtime/rtdata  \
	$(ARCH)/initplat $(ARCH)/math 

ifeq ($(ARCH),ia64)
FILES += $(ARCH)/salpal $(ARCH)/palproc
endif

OBJS  = $(FILES:%=%.o)

libefi.a: libefi.a($(OBJS))

clean:
	rm -f libefi.a *~ $(OBJS) */*.o

install: libefi.a
	mkdir -p $(INSTALLROOT)/lib
	$(INSTALL) -m 644 libefi.a $(INSTALLROOT)/lib/

include ../Make.rules
