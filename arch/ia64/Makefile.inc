IA-64_TARGET=ia64-pc-linux-gnu

IA-64_CC_DIR=/usr/local/ia64/bin
IA-64_BINUTILS_DIR=/usr/local/ia64/bin

CC=$(IA-64_CC_DIR)/$(IA-64_TARGET)-gcc
AS=$(IA-64_BINUTILS_DIR)/$(IA-64_TARGET)-as
LD=$(IA-64_BINUTILS_DIR)/$(IA-64_TARGET)-ld
OBJDUMP=$(IA-64_BINUTILS_DIR)/$(IA-64_TARGET)-objdump
OBJCOPY=$(IA-64_BINUTILS_DIR)/$(IA-64_TARGET)-objcopy
BFD_NAME=elf64-little
BFD_ARCH=ia64-elf64


ASFLAGS=-mconstant-gp

DEFS=-DARCH=$(ARCH)
CFLAGS=$(DEFS) -mconstant-gp -nostdlib -fno-builtin -fno-unwind-tables -O3
LFLAGS=-EL -M



arch_sources= \
	generic/src/arch/start.S \
	generic/src/arch/asm.S \
	generic/src/arch/dummy.s \
	generic/src/arch/putchar.c \
	generic/src/arch/ia64.c \
	generic/src/arch/fpu_context.c \
	generic/src/arch/context.S \
	generic/src/arch/ski/ski.c \
	generic/src/arch/cpu/cpu.c \
	generic/src/arch/ivt.S \
	generic/src/arch/interrupt.c \
	generic/src/arch/interrupt_handler.c \
	generic/src/arch/fmath.c \
	generic/src/arch/mm/frame.c \
	generic/src/arch/drivers/it.c
