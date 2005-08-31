PPC_BINUTILS_DIR=/usr/local/ppc/bin
PPC_TARGET=ppc-linux-gnu

.PHONY: nothing build

nothing:

build: boot.bin
	cp boot.bin ../../../src/load.bin

CC=$(PPC_BINUTILS_DIR)/$(PPC_TARGET)-gcc
AS=$(PPC_BINUTILS_DIR)/$(PPC_TARGET)-as
LD=$(PPC_BINUTILS_DIR)/$(PPC_TARGET)-ld

CPPFLAGS=$(DEFS) -nostdinc -I../include
CFLAGS=$(CPPFLAGS) -nostdlib -fno-builtin -Werror-implicit-function-declaration -Wmissing-prototypes -Werror -O2
LFLAGS=-no-check-sections -N -T _link.ld

boot.bin: boot.o main.o
	$(LD) $(LFLAGS) boot.o main.o -o $@

boot.o: boot.S
	$(CC) $(CFLAGS) -c boot.S -o $@

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o $@

clean:
	-rm *.o *.bin
