AMD64_TARGET=amd64-linux-gnu
AMD64_CC_DIR=/usr/local/amd64/bin
AMD64_BINUTILS_DIR=/usr/local/amd64/bin

CC=$(AMD64_CC_DIR)/$(AMD64_TARGET)-gcc
AS=$(AMD64_BINUTILS_DIR)/$(AMD64_TARGET)-as
LD=$(AMD64_BINUTILS_DIR)/$(AMD64_TARGET)-ld

#ASFLAGS=--64

DEFS=-DARCH=$(ARCH)
CPPFLAGS=$(DEFS) -nostdinc -I../include
CFLAGS=$(CPPFLAGS) -nostdlib -fno-builtin -fno-unwind-tables -Wmissing-prototypes -Werror -O3 -march=opteron -m64 -mcmodel=kernel
LFLAGS=-M -T ../arch/amd64/_link.ld

arch_sources = arch/dummy.s \
	arch/fpu_context.c \
	arch/putchar.c \
	arch/boot/boot.S
