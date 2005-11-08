.PHONY: build clean

CFLAGS = -nostdinc -nostdlib -fno-builtin -Werror-implicit-function-declaration -Wmissing-prototypes -Werror -O3 -I../include

build: boot.bin
	cp boot.bin ../../../load.bin

boot.bin: boot.a.out
	dd if=/dev/zero of=$@ bs=512 count=16
	dd if=boot.a.out of=$@ bs=512 seek=1 conv=notrunc

boot.a.out: boot.o
	$(LD) -no-check-sections -N -T _link.ld -s boot.o -o $@

boot.o: boot.S
	$(CC) $(CFLAGS) -c boot.S -o $@

clean:
	-rm -f boot.o boot.a.out boot.bin ../../../load.bin
