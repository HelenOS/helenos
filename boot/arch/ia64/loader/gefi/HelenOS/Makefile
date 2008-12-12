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
prefix=$(PREFIX)
include ../Make.defaults
CDIR=$(TOPDIR)/..
LINUX_HEADERS	= /usr/src/sys/build
CPPFLAGS	+= -D__KERNEL__ -I$(LINUX_HEADERS)/include
CRTOBJS		= ../gnuefi/crt0-efi-$(ARCH).o
LDSCRIPT	= ../gnuefi/elf_$(ARCH)_efi.lds
LDFLAGS		+= -T $(LDSCRIPT) -shared -Bsymbolic -L../lib -L../gnuefi $(CRTOBJS)
#LOADLIBES	= -lefi -lgnuefi $(shell $(CC) -print-libgcc-file-name)
LOADLIBES	= -lefi -lgnuefi
FORMAT		= efi-app-$(ARCH)


all:	gefi hello.efi


clean:
	rm -f *.efi *~ *.o *.so *.map *.disass *.bin

.PHONY: install

hello.efi: hello.so
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
		   -j .rela -j .reloc --target=$(FORMAT) hello.so hello.efi
	$(OBJDUMP) -d hello.efi > hello.disass  

#When selected first lines or second lines, select if image is linked into hello or not - usefull for network boot
#hello.so: hello.o image.o division.o
hello.so: hello.o image.bin division.o
#	$(LD) $(LDFLAGS) -Map hello.map hello.o division.o image.o -o hello.so $(LOADLIBES) #link image inside hello
	$(LD) $(LDFLAGS) -Map hello.map hello.o division.o -o hello.so $(LOADLIBES) #dont link image inside hello

hello.o: hello.c
	$(CC) $(INCDIR) $(CFLAGS) $(CPPFLAGS) -c hello.c -o hello.o

division.o: division.c
	$(CC) $(INCDIR) $(CFLAGS) $(CPPFLAGS) -c division.c -o division.o


image.bin: ../../image.boot 
	$(OBJCOPY) -O binary ../../image.boot image.bin


image.o: ../../image.boot mkimage
	$(OBJCOPY) -O binary ../../image.boot image.bin
	./mkimage
	$(CC) $(INCDIR) $(CFLAGS) $(CPPFLAGS) -c image.c -o image.o
#	$(OBJCOPY) -I binary -O elf64-ia64-little -B ia64  image.bin image.o

mkimage: mkimage.c
	gcc -o mkimage mkimage.c


gefi:
	make -C .. prefix=$(PREFIX)
