include ../../arch/$(ARCH)/Makefile.inc

sources=libc.c \
	entry.s

CFLAGS+=-nostdinc -I. -Werror-implicit-function-declaration -Wmissing-prototypes -Werror

objects:=$(addsuffix .o,$(basename $(sources)))

.PHONY : all depend build clean dist-clean

all: dist-clean depend build

-include Makefile.depend

depend:
	$(CC) $(CFLAGS) -M $(sources) >Makefile.depend

build: libc.a

clean:
	find . -name '*.o' -exec rm \{\} \;
	-rm libc.a

dist-clean:
	-rm Makefile.depend
	-$(MAKE) clean

libc.a: $(objects)
	$(AR) rc libc.a $(objects)

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
