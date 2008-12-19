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

include Make.defaults

SUBDIRS = lib gnuefi inc

all:	check_gcc $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	rm -f *~
	@for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done

install:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d install; done

.PHONY:	$(SUBDIRS) clean depend

#
# on both platforms you must use gcc 3.0 or higher 
#
check_gcc:
ifeq ($(GCC_VERSION),2)
	@echo "you need to use a version of gcc >= 3.0, you are using `$(CC) --version`"
	@exit 1
endif

include Make.rules
