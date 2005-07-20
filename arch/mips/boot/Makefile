MIPS_BINUTILS_DIR=/usr/local/mipsel/bin
MIPS_TARGET=mipsel-linux-gnu

.PHONY: nothing build

nothing:

build: boot.bin
	cp boot.bin ../../../src/load.bin

AS=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-as
LD=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-ld

ASFLAGS=-mips2
LFLAGS=--oformat=binary -mips2 -e start

boot.bin: boot.o
	$(LD) $(LFLAGS) boot.o -o $@

boot.o:
	$(AS) boot.s -o $@

clean:
	-rm *.o *.bin
