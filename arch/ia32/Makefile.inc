ifeq (${NATIVE_COMPILER},yes)
	CC=gcc
	AS=as
	LD=ld
	OBJCOPY=objcopy
	OBJDUMP=objdump
else
	IA-32_TARGET=i686-pc-linux-gnu

	IA-32_CC_DIR=/usr/local/i686/bin
	IA-32_BINUTILS_DIR=/usr/local/i686/bin

	CC=$(IA-32_CC_DIR)/$(IA-32_TARGET)-gcc
	AS=$(IA-32_BINUTILS_DIR)/$(IA-32_TARGET)-as
	LD=$(IA-32_BINUTILS_DIR)/$(IA-32_TARGET)-ld
	OBJCOPY=$(IA-32_BINUTILS_DIR)/$(IA-32_TARGET)-objcopy
	OBJDUMP=$(IA-32_BINUTILS_DIR)/$(IA-32_TARGET)-objdump
endif

BFD_NAME=elf32-i386
BFD_ARCH=i386

DEFS:=-DARCH=$(ARCH) -DFPU_LAZY

ifeq (${STRONG_ORDERING},yes)
	DEFS+=-D__STRONG_ORDERING__
endif

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
	generic/src/arch/context.s \
	generic/src/arch/debug/panic.s \
	generic/src/arch/delay.s \
	generic/src/arch/asm.S \
	generic/src/arch/proc/scheduler.c \
	generic/src/arch/bios/bios.c \
	generic/src/arch/smp/ap.S \
	generic/src/arch/smp/apic.c \
	generic/src/arch/smp/mps.c \
	generic/src/arch/smp/smp.c \
	generic/src/arch/atomic.S \
	generic/src/arch/smp/ipi.c \
	generic/src/arch/ia32.c \
	generic/src/arch/interrupt.c \
	generic/src/arch/pm.c \
	generic/src/arch/userspace.c \
	generic/src/arch/cpu/cpu.c \
	generic/src/arch/mm/frame.c \
	generic/src/arch/mm/memory_init.c \
	generic/src/arch/mm/page.c \
	generic/src/arch/mm/tlb.c \
	generic/src/arch/drivers/i8042.c \
	generic/src/arch/drivers/i8254.c \
	generic/src/arch/drivers/i8259.c \
	generic/src/arch/drivers/ega.c \
	generic/src/arch/boot/boot.S \
	generic/src/arch/boot/memmap.S\
	generic/src/arch/fpu_context.c\
	generic/src/arch/fmath.c
