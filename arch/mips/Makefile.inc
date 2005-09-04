MIPS_TARGET=mipsel-linux-gnu

MIPS_CC_DIR=/usr/local/mipsel/bin
MIPS_BINUTILS_DIR=/usr/local/mipsel/bin

CC=$(MIPS_CC_DIR)/$(MIPS_TARGET)-gcc
AS=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-as
LD=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-ld
OBJCOPY=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-objcopy
BFD_NAME=elf32-tradlittlemips
BFD_ARCH=mips

ASFLAGS=-mips3

DEFS=-DARCH=$(ARCH)
CPPFLAGS=$(DEFS) -mno-abicalls -nostdinc -I../include
CFLAGS=$(CPPFLAGS) -mips2 -G 0 -nostdlib -fno-builtin -Werror-implicit-function-declaration -Wmissing-prototypes -Werror -O2 -msoft-float
LFLAGS=-mips2 -M -no-check-sections

arch_sources= \
	arch/start.S \
	arch/context.S \
	arch/panic.s \
	arch/mips.c \
	arch/dummy.s \
	arch/putchar.c \
	arch/asm.s \
	arch/exception.c \
	arch/interrupt.c \
	arch/cache.c \
	arch/cpu/cpu.c \
	arch/mm/frame.c \
	arch/mm/page.c \
	arch/mm/tlb.c \
	arch/fpu_context.c \
	arch/fmath.c
