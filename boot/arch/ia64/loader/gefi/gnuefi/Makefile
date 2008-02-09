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
FILES	= reloc_$(ARCH)

OBJS	= $(FILES:%=%.o)

TARGETS	= crt0-efi-$(ARCH).o libgnuefi.a

all:	$(TARGETS)

libgnuefi.a: libgnuefi.a($(OBJS))

clean:
	rm -f $(TARGETS) *~ *.o $(OBJS)

install:
	mkdir -p $(INSTALLROOT)/lib
	$(INSTALL) -m 644 $(TARGETS) $(INSTALLROOT)/lib
	$(INSTALL) -m 644 elf_$(ARCH)_efi.lds $(INSTALLROOT)/lib

include ../Make.rules
