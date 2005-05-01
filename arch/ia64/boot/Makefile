IA-64_BINUTILS_DIR=/usr/local/ia64/bin
IA-64_TARGET=ia64-pc-linux-gnu

.PHONY: nothing build

nothing:

build: boot.bin
	cp boot.bin ../../../src/load.bin

AS=$(IA-64_BINUTILS_DIR)/$(IA-64_TARGET)-as
LD=$(IA-64_BINUTILS_DIR)/$(IA-64_TARGET)-ld

ASFLAGS=-EL
LFLAGS=-EL -T _link.ld -n

boot.bin: boot.o
	$(LD) $(LFLAGS) boot.o -o $@

boot.o:
	$(AS) boot.s -o $@

clean:
	-rm *.o *.bin
