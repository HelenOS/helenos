MIPS_BINUTILS_DIR=/usr/local/mips/bin
MIPS_TARGET=decstation-ultrix

.PHONY: nothing build

nothing:

build: boot.bin
	cp boot.bin ../../../src/load.bin

AS=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-as
LD=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-ld

ASFLAGS=-mips2 -EL
LFLAGS=--oformat=binary -mips2 -EL -e start

boot.bin: boot.o
	$(LD) $(LFLAGS) boot.o -o $@

boot.o:
	$(AS) boot.s -o $@

clean:
	-rm *.o *.bin
