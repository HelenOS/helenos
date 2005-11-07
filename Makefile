include ../arch/$(ARCH)/Makefile.inc

sources=init.c

CFLAGS+=-nostdinc -Ilibc -Werror-implicit-function-declaration -Wmissing-prototypes -Werror

objects:=$(addsuffix .o,$(basename $(sources)))

.PHONY : all depend build clean dist-clean

all: dist-clean depend build

-include Makefile.depend

depend:
	$(CC) $(CFLAGS) -M $(sources) >Makefile.depend

build: init

clean:
	find . -name '*.o' -maxdepth 1 -exec rm \{\} \;
	-rm init

dist-clean:
	-rm Makefile.depend
	-$(MAKE) clean

init: $(objects)
	$(LD) -T _link.ld -G 0 -static $(objects) libc/libc.a -o init

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
