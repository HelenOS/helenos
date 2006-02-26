.PHONY: build clean

CFLAGS = -nostdinc -nostdlib -fno-builtin -Werror-implicit-function-declaration -Wmissing-prototypes -Werror -O3 -I../include
DEFS = -DKERNEL_LOAD_ADDRESS=0x800000 -DKERNEL_SIZE=40960

SOURCES = \
	main.c \
	ofw.c \
	printf.c \
	boot.S

OBJECTS := $(addsuffix .o,$(basename $(SOURCES)))

build: boot.bin
	cp boot.bin ../../../load.bin

boot.bin: $(OBJECTS)
	$(LD) -no-check-sections -N -T _link.ld $(OBJECTS) -o $@

%.o: %.S
	$(CC) $(DEFS) $(CFLAGS) -D__ASM__ -c $< -o $@

%.o: %.c
	$(CC) $(DEFS) $(CFLAGS) -c $< -o $@

clean:
	-rm -f $(OBJECTS) boot.bin ../../../load.bin
