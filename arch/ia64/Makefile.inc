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
	src/arch/start.S \
	src/arch/asm.S \
	src/arch/dummy.s \
	src/arch/putchar.c \
	src/arch/ia64.c \
	src/arch/fpu_context.c \
	src/arch/context.S \
	src/arch/ski/ski.c \
	src/arch/cpu/cpu.c \
	src/arch/ivt.S \
	src/arch/interrupt_handler.c \
	src/arch/fmath.c \
	src/arch/mm/frame.c
