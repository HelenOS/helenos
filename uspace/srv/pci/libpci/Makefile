# Makefile for The PCI Library
# (c) 1999 Martin Mares <mj@ucw.cz>

# Modified and ported to HelenOS by Jakub Jermar

-include ../../../../Makefile.config

LIBC_PREFIX=$(shell cd ../../../lib/libc; pwd)

include $(LIBC_PREFIX)/Makefile.toolchain

LIBS = $(LIBC_PREFIX)/libc.a
CFLAGS += -I$(LIBC_PREFIX)/include -trigraphs

OBJS=access.o generic.o names.o
INCL=internal.h pci.h header.h sysdep.h types.h pci_ids.h

PCILIB=libpci.a

OBJS += i386-ports.o

all: $(PCILIB)

$(PCILIB): $(OBJS)
	$(AR) rc $@ $(OBJS)

%.o: %.c $(INCL)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm *.o libpci.a
