.PHONY: all clean fasterclean all-test

FILES := $(wildcard *.c)
LAYOUTS := $(patsubst %.c, %, ${FILES}) 

all:
	$(foreach LAYOUT,$(LAYOUTS), $(MAKE) -f Makefile.helper LAYOUT=$(LAYOUT) $@;)
	$(foreach LAYOUT,$(LAYOUTS), cp -p $(LAYOUT).so.0.0 $(LAYOUT).so;)

clean:
	$(foreach LAYOUT,$(LAYOUTS), $(MAKE) -f Makefile.helper LAYOUT=$(LAYOUT) $@;)
	rm *.so

fasterclean:
	$(foreach LAYOUT,$(LAYOUTS), $(MAKE) -f Makefile.helper LAYOUT=$(LAYOUT) $@;)
	rm *.so

all-test: