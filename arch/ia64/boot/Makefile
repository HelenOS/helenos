.PHONY: build clean

build: boot.bin
	cp boot.bin ../../../load.bin

boot.bin: boot.o
	$(LD) -EL -T _link.ld -n boot.o -o $@

boot.o: boot.s
	$(AS) boot.s -o $@

clean:
	-rm -f boot.o boot.bin ../../../load.bin
