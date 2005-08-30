IA-64_TARGET=ia64-pc-linux-gnu

IA-64_CC_DIR=/usr/local/ia64/bin
IA-64_BINUTILS_DIR=/usr/local/ia64/bin

CC=$(IA-64_CC_DIR)/$(IA-64_TARGET)-gcc
AS=$(IA-64_BINUTILS_DIR)/$(IA-64_TARGET)-as
LD=$(IA-64_BINUTILS_DIR)/$(IA-64_TARGET)-ld

ASFLAGS=-mconstant-gp

DEFS=-DARCH=$(ARCH)
CPPFLAGS=$(DEFS) -nostdinc -I../include
CFLAGS=$(CPPFLAGS) -mconstant-gp -nostdlib -fno-builtin -fno-unwind-tables -Werror-implicit-function-declaration -Wmissing-prototypes -Werror -O3
LFLAGS=-EL -M -T ../arch/ia64/_link.ld

arch_sources= \
	arch/start.S \
	arch/asm.S \
	arch/dummy.s \
	arch/putchar.c \
	arch/ia64.c \
	arch/fpu_context.c \
	arch/context.S \
	arch/ski/ski.c
