PPC_TARGET=ppc-linux-gnu

PPC_CC_DIR=/usr/local/ppc/bin
PPC_BINUTILS_DIR=/usr/local/ppc/bin

CC=$(PPC_CC_DIR)/$(PPC_TARGET)-gcc
AS=$(PPC_BINUTILS_DIR)/$(PPC_TARGET)-as
LD=$(PPC_BINUTILS_DIR)/$(PPC_TARGET)-ld
OBJDUMP=$(PPC_BINUTILS_DIR)/$(PPC_TARGET)-objdump
OBJCOPY=$(PPC_BINUTILS_DIR)/$(PPC_TARGET)-objcopy
BFD_NAME=elf32-powerpc
BFD_ARCH=powerpc

ASFLAGS=

DEFS=-DARCH=$(ARCH)
CFLAGS=$(DEFS) -nostdlib -fno-builtin -O2
LFLAGS=-M -no-check-sections -N

arch_sources= \
	arch/context.S \
	arch/debug/panic.s \
	arch/fpu_context.c \
	arch/ppc.c \
	arch/dummy.s \
	arch/start.S \
	arch/asm.S \
	arch/cpu/cpu.c \
	arch/mm/frame.c \
	arch/mm/memory_init.c \
	arch/mm/page.c \
	arch/drivers/ofw.c \
	arch/fmath.c
