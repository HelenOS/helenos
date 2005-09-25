ifeq (${CROSS_COMPILER},yes)
	IA-32_TARGET=i686-pc-linux-gnu

	IA-32_CC_DIR=/usr/local/i686/bin
	IA-32_BINUTILS_DIR=/usr/local/i686/bin

	CC=$(IA-32_CC_DIR)/$(IA-32_TARGET)-gcc
	AS=$(IA-32_BINUTILS_DIR)/$(IA-32_TARGET)-as
	LD=$(IA-32_BINUTILS_DIR)/$(IA-32_TARGET)-ld
	OBJCOPY=$(IA-32_BINUTILS_DIR)/$(IA-32_TARGET)-objcopy
	OBJDUMP=$(IA-32_BINUTILS_DIR)/$(IA-32_TARGET)-objdump
else
	CC=gcc
	AS=as
	LD=ld
	OBJCOPY=objcopy
	OBJDUMP=objdump
endif

BFD_NAME=elf32-i386
BFD_ARCH=i386

DEFS:=-DARCH=$(ARCH) -DFPU_LAZY

ifdef SMP
DEFS+=-D$(SMP)
endif

ifdef HT
DEFS+=-D$(HT)
endif

CPPFLAGS=$(DEFS) -nostdinc -Iinclude/
CFLAGS=$(CPPFLAGS) -nostdlib -fno-builtin -fomit-frame-pointer -Werror-implicit-function-declaration -Wmissing-prototypes -Werror -O3
LFLAGS=-M

arch/$(ARCH)/_link.ld: arch/$(ARCH)/_link.ld.in
	$(CC) $(CFLAGS) -E -x c $< | grep -v "^\#" > $@

arch_sources= \
	src/arch/context.s \
	src/arch/debug/panic.s \
	src/arch/cpuid.s \
	src/arch/delay.s \
	src/arch/asm.S \
	src/arch/proc/scheduler.c \
	src/arch/acpi/acpi.c \
	src/arch/acpi/madt.c \
	src/arch/bios/bios.c \
	src/arch/smp/ap.S \
	src/arch/smp/apic.c \
	src/arch/smp/mps.c \
	src/arch/smp/smp.c \
	src/arch/atomic.S \
	src/arch/smp/ipi.c \
	src/arch/ia32.c \
	src/arch/interrupt.c \
	src/arch/pm.c \
	src/arch/userspace.c \
	src/arch/cpu/cpu.c \
	src/arch/mm/frame.c \
	src/arch/mm/memory_init.c \
	src/arch/mm/page.c \
	src/arch/mm/tlb.c \
	src/arch/drivers/i8042.c \
	src/arch/drivers/i8254.c \
	src/arch/drivers/i8259.c \
	src/arch/drivers/ega.c \
	src/arch/boot/boot.S \
	src/arch/boot/memmap.S\
	src/arch/fpu_context.c\
	src/arch/fmath.c
