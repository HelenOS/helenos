.PHONY: all clean fasterclean all-test

FILES := $(wildcard *.c)
LAYOUTS := $(patsubst %.c, %, ${FILES}) 

all:
	$(foreach LAYOUT,$(LAYOUTS), $(MAKE) -f Makefile.helper LAYOUT=$(LAYOUT) $@;)
	

clean:
	$(foreach LAYOUT,$(LAYOUTS), $(MAKE) -f Makefile.helper LAYOUT=$(LAYOUT) $@;)

fasterclean:
	$(foreach LAYOUT,$(LAYOUTS), $(MAKE) -f Makefile.helper LAYOUT=$(LAYOUT) $@;)

all-test: