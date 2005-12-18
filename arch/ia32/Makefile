.PHONY=build clean

ROOT=distroot
BASE=$(shell cd ../../..; pwd)
KERNELDIR=$(BASE)/kernel
USPACEDIR=$(BASE)/uspace

build: $(KERNELDIR)/kernel.bin $(USPACEDIR)/init/init
	gunzip -c grub/grub.img.gz > image.bin
	e2cp $(KERNELDIR)/kernel.bin image.bin:/boot/kernel.bin
	e2cp $(USPACEDIR)/init/init image.bin:/init
	e2cp grub/README grub/COPYING image.bin:/boot

$(KERNELDIR)/kernel.bin:
	$(MAKE) -C $(KERNELDIR)

$(USPACEDIR)/init/init:
	$(MAKE) -C $(USPACEDIR)

clean:
	-rm image.bin
