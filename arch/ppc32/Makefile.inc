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
	src/arch/console.c \
	src/arch/context.S \
	src/arch/debug/panic.s \
	src/arch/fpu_context.c \
	src/arch/ppc32.c \
	src/arch/dummy.s \
	src/arch/start.S \
	src/arch/asm.S \
	src/arch/cpu/cpu.c \
	src/arch/mm/frame.c \
	src/arch/mm/memory_init.c \
	src/arch/mm/page.c \
	src/arch/fmath.c
