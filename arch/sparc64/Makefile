.PHONY=build kernel clean

ROOT=distroot
BASE=$(shell cd ../../..; pwd)
KERNELDIR=$(BASE)/kernel

build: kernel
	mkdir -p $(ROOT)/boot
	mkdir -p $(ROOT)/kernel
	cat silo/silo.tar.gz | (cd $(ROOT)/boot; tar xvfz -)
	cp silo/README silo/COPYING silo/silo.conf $(ROOT)/boot
	ln -s $(KERNELDIR)/kernel.bin $(ROOT)/kernel
	mkisofs -f -G distroot/boot/isofs.b -B ... -r -o image.iso $(ROOT)/

kernel:
	$(MAKE) -C $(KERNELDIR)

clean:
	-rm -r $(ROOT)/
	-rm image.iso
