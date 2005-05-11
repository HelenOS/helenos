# decstation-ultrix target has been obsoleted in recent gcc compilers (3.1) and removed in (3.3)
MIPS_TARGET=decstation-ultrix

MIPS_CC_DIR=/usr/local/mips/bin
MIPS_BINUTILS_DIR=/usr/local/mips/bin

CC=$(MIPS_CC_DIR)/$(MIPS_TARGET)-gcc
AS=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-as
LD=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-ld

ASFLAGS=-EL -mips3

DEFS=-DARCH=$(ARCH)
CPPFLAGS=$(DEFS) -nostdinc -I../include
CFLAGS=$(CPPFLAGS) -EL -mips2 -G 0 -nostdlib -fno-builtin -Wmissing-prototypes -Werror -O2
LFLAGS=-EL -mips2 -M -no-check-sections -T ../arch/mips/_link.ld

arch_sources= \
	arch/start.S \
	arch/context.S \
	arch/panic.s \
	arch/mips.c \
	arch/fake.s \
	arch/putchar.c \
	arch/asm.s \
	arch/exception.c \
	arch/interrupt.c \
	arch/cache.c \
	arch/cpu/cpu.c \
	arch/mm/frame.c \
	arch/mm/page.c \
	arch/mm/tlb.c \
	arch/fpu_context.c
