BINUTILS_DIR=/usr/local/sparc64/bin
TARGET=sparc64-linux-gnu

.PHONY: nothing build

nothing:

build: boot.bin
	cp boot.bin ../../../load.bin

CC=$(BINUTILS_DIR)/$(TARGET)-gcc
AS=$(BINUTILS_DIR)/$(TARGET)-as
LD=$(BINUTILS_DIR)/$(TARGET)-ld

CPPFLAGS=$(DEFS) -nostdinc -I../include
CFLAGS=$(CPPFLAGS) -nostdlib -fno-builtin -Werror-implicit-function-declaration -Wmissing-prototypes -Werror -O2
LFLAGS=-no-check-sections -N -T _link.ld -s

boot.bin: boot.aout
	dd if=/dev/zero of=$@ bs=512 count=16
	dd if=boot.aout of=$@ bs=512 seek=1 conv=notrunc

boot.aout: boot.o
	$(LD) $(LFLAGS) boot.o -o $@

boot.o: boot.S
	$(CC) $(CFLAGS) -c boot.S -o $@

clean:
	-rm *.o *.bin *.aout
