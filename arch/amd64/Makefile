.PHONY=build kernel clean

ROOT=distroot
BASE=$(shell cd ../../..; pwd)
KERNELDIR=$(BASE)/kernel

build: kernel
	gunzip -c grub/grub.img.gz > image.bin
	e2cp $(KERNELDIR)/kernel.bin image.bin:/boot/kernel.bin

kernel:
	$(MAKE) -C $(KERNELDIR)

clean:
	-rm image.bin
