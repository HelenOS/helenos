PPC_CC_DIR=/usr/local/ppc/bin
CC=$(PPC_CC_DIR)/ppc-linux-gnu-gcc
AS=$(PPC_CC_DIR)/ppc-linux-gnu-as
LD=$(PPC_CC_DIR)/ppc-linux-gnu-ld

ASFLAGS=

DEFS=-DARCH=$(ARCH)
CPPFLAGS=$(DEFS) -nostdinc -I../include
CFLAGS=$(CPPFLAGS) -nostdlib -fno-builtin -Wmissing-prototypes -Werror -O2
LFLAGS=-M -no-check-sections -T ../arch/ppc/_link.ld

arch_sources= \
	arch/powerpc.c \
	arch/fpu_context.c
