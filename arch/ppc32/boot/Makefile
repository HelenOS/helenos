.PHONY: build clean

CFLAGS = -nostdinc -nostdlib -fno-builtin -Werror-implicit-function-declaration -Wmissing-prototypes -Werror -O3 -I../include

build: boot.bin
	cp boot.bin ../../../load.bin

boot.bin: boot.o main.o
	$(LD) -no-check-sections -N -T _link.ld boot.o main.o -o $@

boot.o: boot.S
	$(CC) $(CFLAGS) -c boot.S -o $@

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o $@

clean:
	-rm -f boot.o main.o boot.bin ../../../load.bin
