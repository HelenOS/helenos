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
	generic/src/arch/console.c \
	generic/src/arch/context.S \
	generic/src/arch/debug/panic.s \
	generic/src/arch/fpu_context.c \
	generic/src/arch/ppc32.c \
	generic/src/arch/dummy.s \
	generic/src/arch/start.S \
	generic/src/arch/asm.S \
	generic/src/arch/cpu/cpu.c \
	generic/src/arch/mm/frame.c \
	generic/src/arch/mm/memory_init.c \
	generic/src/arch/mm/page.c \
	generic/src/arch/fmath.c
