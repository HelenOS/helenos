.PHONY: nothing build

nothing:

build: boot.bin
	cp boot.bin ../../../src/load.bin

AS=/usr/local/mips/bin/decstation-ultrix-as
LD=/usr/local/mips/bin/decstation-ultrix-ld

ASFLAGS=-mips2 -EL
LFLAGS=--oformat=binary -mips2 -EL -e start

boot.bin: boot.o
	$(LD) $(LFLAGS) boot.o -o $@

boot.o:
	$(AS) boot.s -o $@

clean:
	-rm *.o *.bin
