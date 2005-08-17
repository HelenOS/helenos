PPC_BINUTILS_DIR=/usr/local/ppc/bin
PPC_TARGET=ppc-linux-gnu

.PHONY: nothing build

nothing:

build: boot.bin
	cp boot.bin ../../../src/load.bin

AS=$(PPC_BINUTILS_DIR)/$(PPC_TARGET)-as
LD=$(PPC_BINUTILS_DIR)/$(PPC_TARGET)-ld

ASFLAGS=
LFLAGS=--oformat=binary -e start

boot.bin: boot.o
	$(LD) $(LFLAGS) boot.o -o $@

boot.o:
	$(AS) boot.s -o $@

clean:
	-rm *.o *.bin
