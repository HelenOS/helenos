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
	arch/start.S \
	arch/asm.S \
	arch/dummy.s \
	arch/putchar.c \
	arch/ia64.c \
	arch/fpu_context.c \
	arch/context.S \
	arch/ski/ski.c \
	arch/cpu/cpu.c \
	arch/ivt.S \
	arch/interrupt_handler.c \
	arch/fmath.c
