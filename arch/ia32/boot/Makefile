.PHONY: build clean

build: grub.img.gz
	gunzip -c grub.img.gz > ../../../image.bin
	e2cp ../../../kernel.bin ../../../image.bin:/boot/kernel.bin

clean:
	-rm -f ../../../image.bin
