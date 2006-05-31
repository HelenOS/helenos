LIBC_PREFIX = ../libc
SOFTINT_PREFIX = ../softint
include $(LIBC_PREFIX)/Makefile.toolchain

LIBS = $(LIBC_PREFIX)/libc.a

OUTPUT = tetris
SOURCES = shapes.c scores.c input.c tetris.c screen.c
OBJECTS := $(addsuffix .o,$(basename $(SOURCES)))

.PHONY: all clean depend disasm

all: $(OUTPUT)

-include Makefile.depend

depend:
	$(CC) $(DEFS) $(CFLAGS) -M $(SOURCES) > Makefile.depend

$(OUTPUT): $(OBJECTS) $(LIBS)
	$(LD) -T $(LIBC_PREFIX)/arch/$(ARCH)/_link.ld $(OBJECTS) $(LIBS) $(LFLAGS) -o $@ -Map $(OUTPUT).map

clean:
	-rm -f $(OUTPUT) $(OUTPUT).map $(OUTPUT).disasm Makefile.depend *.o
disasm:
	$(OBJDUMP) -d $(OUTPUT) >$(OUTPUT).disasm

%.o: %.S
	$(CC) $(DEFS) $(AFLAGS) $(CFLAGS) -D__ASM__ -c $< -o $@

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o: %.c
	$(CC) $(DEFS) $(CFLAGS) -c $< -o $@
