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
CDIR=$(TOPDIR)/..
LINUX_HEADERS	= /usr/src/sys/build
CPPFLAGS	+= -D__KERNEL__ -I$(LINUX_HEADERS)/include
CRTOBJS		= ../gnuefi/crt0-efi-$(ARCH).o
LDSCRIPT	= ../gnuefi/elf_$(ARCH)_efi.lds
LDFLAGS		+= -T $(LDSCRIPT) -shared -Bsymbolic -L../lib -L../gnuefi $(CRTOBJS)
LOADLIBES	= -lefi -lgnuefi
FORMAT		= efi-app-$(ARCH)

TARGETS = t.efi t2.efi t3.efi t4.efi t5.efi t6.efi printenv.efi t7.efi

all:	$(TARGETS)


clean:
	rm -f $(TARGETS) *~ *.o *.so

.PHONY: install

include ../Make.rules
