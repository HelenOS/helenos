.PHONY: nothing build clean

nothing:

build: boot.bin
	dd if=boot.bin of=../../../src/image.bin bs=512 conv=sync
	-cat ../../../src/kernel.bin >>../../../src/image.bin
	dd if=/dev/zero of=../../../src/image.bin bs=1 seek=$(KERNEL_SIZE) count=`expr 1474560 - $(KERNEL_SIZE)`

boot.bin: boot.o
	ld -T boot.ld -entry _start_0x7c00 --oformat binary boot.o -o $@

boot.o: boot.S
	gcc -E -DKERNEL_SIZE=$(KERNEL_SIZE) boot.S >boot.s
	as boot.s -o $@
	rm boot.s

clean:
	-rm *.o *.bin
