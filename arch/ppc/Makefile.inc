PPC_TARGET=ppc-linux-gnu

PPC_CC_DIR=/usr/local/ppc/bin
PPC_BINUTILS_DIR=/usr/local/ppc/bin

CC=$(PPC_CC_DIR)/$(PPC_TARGET)-gcc
AS=$(PPC_BINUTILS_DIR)/$(PPC_TARGET)-as
LD=$(PPC_BINUTILS_DIR)/$(PPC_TARGET)-ld

ASFLAGS=

DEFS=-DARCH=$(ARCH)
CPPFLAGS=$(DEFS) -nostdinc -I../include
CFLAGS=$(CPPFLAGS) -nostdlib -fno-builtin -Werror-implicit-function-declaration -Wmissing-prototypes -Werror -O2
LFLAGS=-M -no-check-sections -T ../arch/ppc/_link.ld

arch_sources= \
	arch/context.S \
	arch/debug/panic.s \
	arch/fpu_context.c \
	arch/dummy.s \
	arch/start.S \
	arch/asm.s \
	arch/mm/frame.c \
	arch/mm/memory_init.c \
	arch/mm/page.c \
	arch/drivers/ofw.c
