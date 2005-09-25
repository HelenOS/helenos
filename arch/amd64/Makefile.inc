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
	src/arch/dummy.s \
	src/arch/fpu_context.c \
	src/arch/boot/boot.S \
	src/arch/boot/memmap.S \
	src/arch/pm.c \
	src/arch/context.S \
	src/arch/drivers/ega.c \
	src/arch/drivers/i8042.c \
	src/arch/drivers/i8254.c \
	src/arch/drivers/i8259.c \
	src/arch/supplib.c \
	src/arch/delay.S \
	src/arch/amd64.c \
	src/arch/bios/bios.c \
	src/arch/interrupt.c \
	src/arch/mm/frame.c \
	src/arch/mm/page.c \
	src/arch/mm/tlb.c \
	src/arch/asm_utils.S \
	src/arch/fmath.c \
	src/arch/mm/memory_init.c \
	src/arch/cpu/cpu.c \
	src/arch/proc/scheduler.c \
	src/arch/userspace.c \
	src/arch/acpi/acpi.c \
	src/arch/acpi/madt.c

ifdef SMP
arch_sources += src/arch/smp/ap.S \
		src/arch/smp/apic.c \
		src/arch/smp/ipi.c \
		src/arch/smp/mps.c \
		src/arch/smp/smp.c
endif
