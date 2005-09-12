MIPS_TARGET=mipsel-linux-gnu
MIPS_CC_DIR=/usr/local/mipsel/bin
MIPS_BINUTILS_DIR=/usr/local/mipsel/bin

CC=$(MIPS_CC_DIR)/$(MIPS_TARGET)-gcc
AS=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-as
LD=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-ld
OBJDUMP=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-objdump
OBJCOPY=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-objcopy
BFD_ARCH=mips

DEFS=-DARCH=$(ARCH) -DMACHINE=${MACHINE} -DKERNEL_LOAD_ADDRESS=${KERNEL_LOAD_ADDRESS}
CFLAGS=$(DEFS) -mno-abicalls -G 0 -nostdlib -fno-builtin -O2  -fno-zero-initialized-in-bss 
LFLAGS=-M -N
BFD_ARCH=mips

# GCC 4.0.1 compiled for mipsEL has problems compiling in 
# BigEndian mode with the swl/swr/lwl/lwr instructions.
# We have to compile it with mips-sgi-irix5 to get it right.
ifeq (${MACHINE},indy)
 MIPS_TARGET=mips-sgi-irix5
 MIPS_CC_DIR=/usr/local/mips/bin
 MIPS_BINUTILS_DIR=/usr/local/mips/bin

 CFLAGS += -EB -DBIG_ENDIAN -DHAVE_FPU -DFPU_LAZY -march=r4600
 BFD = ecoff-bigmips
 KERNEL_LOAD_ADDRESS = 0x88002000
 BFD_NAME=elf32-bigmips
endif

ifeq (${MACHINE},lgxemul)
 CFLAGS += -DHAVE_FPU -DFPU_LAZY -mips3
 BFD = ecoff-littlemips
 KERNEL_LOAD_ADDRESS = 0x80100000
 BFD_NAME=elf32-little
endif

ifeq (${MACHINE},bgxemul)
 MIPS_TARGET=mips-sgi-irix5
 MIPS_CC_DIR=/usr/local/mips/bin
 MIPS_BINUTILS_DIR=/usr/local/mips/bin

 CFLAGS += -EB -DBIG_ENDIAN -DHAVE_FPU -DFPU_LAZY -mips3
 BFD = ecoff-bigmips
 KERNEL_LOAD_ADDRESS = 0x80100000
 BFD_NAME=elf32-bigmips
endif

# MSIM needs lwl/swl patch & 4kc instruction patch to work
# otherwise add -mmemcpy -mips3
ifeq (${MACHINE},msim)
 BFD = binary
 CFLAGS += -msoft-float -march=4kc 
 KERNEL_LOAD_ADDRESS = 0x80100000
 BFD_NAME=elf32-little
endif

# SIMICS 4kc emulation is broken, although for instructions
# that do not bother us
ifeq (${MACHINE},simics)
 BFD = elf32-little
 CFLAGS += -msoft-float -mips3
 KERNEL_LOAD_ADDRESS = 0x80100000
 BFD_NAME=elf32-little
endif

../arch/$(ARCH)/_link.ld: ../arch/$(ARCH)/_link.ld.in
	$(CC) $(CFLAGS) -C -DBFD=${BFD} -E -x c $< | grep -v "^\#" > $@

arch_sources= \
	arch/start.S \
	arch/context.S \
	arch/panic.S \
	arch/mips32.c \
	arch/dummy.S \
	arch/console.c \
	arch/asm.S \
	arch/exception.c \
	arch/interrupt.c \
	arch/cache.c \
	arch/cpu/cpu.c \
	arch/mm/frame.c \
	arch/mm/page.c \
	arch/mm/tlb.c \
	arch/fpu_context.c \
	arch/fmath.c \
	arch/drivers/arc.c
