MIPS_CC_DIR=/usr/local/powerpc/bin
CC=$(MIPS_CC_DIR)/powerpc-linux-gcc
AS=$(MIPS_CC_DIR)/powerpc-linux-as
LD=$(MIPS_CC_DIR)/powerpc-linux-ld

ASFLAGS=

DEFS=-DARCH=$(ARCH)
CPPFLAGS=$(DEFS) -nostdinc -I../include
CFLAGS=$(CPPFLAGS) -nostdlib -fno-builtin -Wmissing-prototypes -Werror -O2
LFLAGS=-M -no-check-sections -T ../arch/mips/_link.ld

arch_sources= \
	arch/powerpc.c
