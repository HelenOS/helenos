AMD64_TARGET=amd64-linux-gnu
AMD64_CC_DIR=/usr/local/amd64/bin
AMD64_BINUTILS_DIR=/usr/local/amd64/bin

CC=$(AMD64_CC_DIR)/$(AMD64_TARGET)-gcc
AS=$(AMD64_BINUTILS_DIR)/$(AMD64_TARGET)-as
LD=$(AMD64_BINUTILS_DIR)/$(AMD64_TARGET)-ld
OBJCOPY=$(AMD64_BINUTILS_DIR)/$(AMD64_TARGET)-objcopy
BFD_NAME=elf64-x86-64
BFD_ARCH=i386:x86-64

DEFS=-DARCH=$(ARCH)

ifdef SMP
DEFS+=-D$(SMP)
endif

ifdef HT
DEFS+=-D$(HT)
endif


CPPFLAGS=$(DEFS) -nostdinc -I../include
CFLAGS=$(CPPFLAGS) -nostdlib -fno-builtin -fno-unwind-tables -Werror-implicit-function-declaration -Wmissing-prototypes -Werror -O3 -march=opteron -m64 -mcmodel=kernel -mno-red-zone
LFLAGS=-M -T ../arch/amd64/_link.ld

arch_sources = arch/dummy.s \
	arch/fpu_context.c \
	arch/boot/boot.S \
	arch/boot/memmap.S \
	arch/pm.c \
	arch/context.S \
	arch/drivers/ega.c \
	arch/drivers/i8042.c \
	arch/drivers/i8254.c \
	arch/drivers/i8259.c \
	arch/supplib.c \
	arch/delay.S \
	arch/amd64.c \
	arch/bios/bios.c \
	arch/interrupt.c \
	arch/mm/frame.c \
	arch/mm/page.c \
	arch/mm/tlb.c \
	arch/asm_utils.S \
	arch/fmath.c \
	arch/mm/memory_init.c \
	arch/cpu/cpu.c \
	arch/proc/scheduler.c \
	arch/userspace.c \
	arch/acpi/acpi.c \
	arch/acpi/madt.c

ifdef SMP
arch_sources += arch/smp/apic.c \
		arch/smp/ipi.c \
		arch/smp/mps.c \
		arch/smp/smp.c
endif
