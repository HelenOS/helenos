.PHONY: build clean

CFLAGS = -nostdinc -nostdlib -fno-builtin -Werror-implicit-function-declaration -Wmissing-prototypes -Werror -O3 -mips3 -I../include

build: boot.bin
	cp boot.bin ../../../load.bin

boot.bin: boot.o
	$(LD) -e start -T _link.ld boot.o -o $@

boot.o: boot.S
	$(CC) $(CFLAGS) -c boot.S -o $@

clean:
	-rm -f boot.o boot.bin ../../../load.bin
