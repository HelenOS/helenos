AMD64_TARGET=amd64-linux-gnu
AMD64_CC_DIR=/usr/local/amd64/bin
AMD64_BINUTILS_DIR=/usr/local/amd64/bin

CC=$(AMD64_CC_DIR)/$(AMD64_TARGET)-gcc
AS=$(AMD64_BINUTILS_DIR)/$(AMD64_TARGET)-as
LD=$(AMD64_BINUTILS_DIR)/$(AMD64_TARGET)-ld
OBJDUMP=$(AMD64_BINUTILS_DIR)/$(AMD64_TARGET)-objdump
OBJCOPY=$(AMD64_BINUTILS_DIR)/$(AMD64_TARGET)-objcopy
BFD_NAME=elf64-x86-64
BFD_ARCH=i386:x86-64

DEFS=-DARCH=$(ARCH) -DFPU_LAZY

ifdef SMP
DEFS+=-D$(SMP)
endif

ifdef HT
DEFS+=-D$(HT)
endif


CFLAGS=$(DEFS) -nostdlib -fno-builtin -fno-unwind-tables -O3 -march=opteron -m64 -mcmodel=kernel -mno-red-zone
LFLAGS=-M 

arch/$(ARCH)/_link.ld: arch/$(ARCH)/_link.ld.in
	$(CC) $(CFLAGS) -C -E -x c $< | grep -v "^\#" > $@

arch_sources = \
	generic/src/arch/dummy.s \
	generic/src/arch/fpu_context.c \
	generic/src/arch/boot/boot.S \
	generic/src/arch/boot/memmap.S \
	generic/src/arch/pm.c \
	generic/src/arch/context.S \
	generic/src/arch/drivers/ega.c \
	generic/src/arch/drivers/i8042.c \
	generic/src/arch/drivers/i8254.c \
	generic/src/arch/drivers/i8259.c \
	generic/src/arch/delay.S \
	generic/src/arch/amd64.c \
	generic/src/arch/bios/bios.c \
	generic/src/arch/interrupt.c \
	generic/src/arch/mm/frame.c \
	generic/src/arch/mm/page.c \
	generic/src/arch/mm/tlb.c \
	generic/src/arch/asm_utils.S \
	generic/src/arch/fmath.c \
	generic/src/arch/mm/memory_init.c \
	generic/src/arch/cpu/cpu.c \
	generic/src/arch/proc/scheduler.c \
	generic/src/arch/userspace.c

ifdef SMP
arch_sources += generic/src/arch/smp/ap.S \
		generic/src/arch/smp/apic.c \
		generic/src/arch/smp/ipi.c \
		generic/src/arch/smp/mps.c \
		generic/src/arch/smp/smp.c
endif
