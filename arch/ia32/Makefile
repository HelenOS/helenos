.PHONY=build clean

ROOT=distroot
BASE=$(shell cd ../../..; pwd)
KERNELDIR=$(BASE)/kernel

build: $(KERNELDIR)/kernel.bin
	gunzip -c grub/grub.img.gz > image.bin
	e2cp $(KERNELDIR)/kernel.bin image.bin:/kernel/kernel.bin
	e2cp grub/README grub/COPYING image.bin:/boot

$(KERNELDIR)/kernel.bin:
	$(MAKE) -C $(KERNELDIR)

clean:
	-rm image.bin
