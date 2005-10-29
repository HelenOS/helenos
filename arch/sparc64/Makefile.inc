TARGET=sparc64-linux-gnu

CC_DIR=/usr/local/sparc64/bin
BINUTILS_DIR=/usr/local/sparc64/bin

CC=$(CC_DIR)/$(TARGET)-gcc
AS=$(BINUTILS_DIR)/$(TARGET)-as
LD=$(BINUTILS_DIR)/$(TARGET)-ld
OBJDUMP=$(BINUTILS_DIR)/$(TARGET)-objdump
OBJCOPY=$(BINUTILS_DIR)/$(TARGET)-objcopy
BFD_NAME=elf64-sparc
BFD_ARCH=sparc

ASFLAGS=

DEFS=-DARCH=$(ARCH)
CFLAGS=$(DEFS) -nostdlib -fno-builtin -O2
LFLAGS=-M -no-check-sections -N

arch_sources= \
	src/arch/dummy.s \
	src/arch/start.S \
	src/arch/sparc64.c

