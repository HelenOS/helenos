MIPS_BINUTILS_DIR=/usr/local/mipsel/bin
MIPS_TARGET=mipsel-linux-gnu

.PHONY: nothing build

nothing:

build: boot.bin
	cp boot.bin ../../../load.bin

AS=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-as
CC=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-gcc
LD=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-ld

AFLAGS=-mips2 -I../../../generic/include
LFLAGS=--oformat=binary -e start -T _link.ld 

.S.o:
	$(CC) $(ASFLAGS) -c -o $@ $<

boot.bin: boot.o
	$(LD) $(LFLAGS) $< -o $@


clean:
	-rm *.o *.bin
