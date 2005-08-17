PPC_TARGET=ppc-linux-gnu

PPC_CC_DIR=/usr/local/ppc/bin
PPC_BINUTILS_DIR=/usr/local/ppc/bin

CC=$(PPC_CC_DIR)/$(PPC_TARGET)-gcc
AS=$(PPC_BINUTILS_DIR)/$(PPC_TARGET)-as
LD=$(PPC_BINUTILS_DIR)/$(PPC_TARGET)-ld

ASFLAGS=

DEFS=-DARCH=$(ARCH)
CPPFLAGS=$(DEFS) -nostdinc -I../include
CFLAGS=$(CPPFLAGS) -nostdlib -fno-builtin -Wmissing-prototypes -Werror -O2
LFLAGS=-M -no-check-sections -T ../arch/ppc/_link.ld

arch_sources= \
	arch/ppc.c \
	arch/fpu_context.c \
	arch/dummy.s \
	arch/start.S \
	arch/asm.s \
	arch/putchar.c \
	arch/mm/frame.c \
	arch/mm/page.c
