MIPS_TARGET=mipsel-linux-gnu

MIPS_CC_DIR=/usr/local/mipsel/bin
MIPS_BINUTILS_DIR=/usr/local/mipsel/bin

CC=$(MIPS_CC_DIR)/$(MIPS_TARGET)-gcc
AS=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-as
LD=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-ld
OBJDUMP=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-objdump
OBJCOPY=$(MIPS_BINUTILS_DIR)/$(MIPS_TARGET)-objcopy
BFD_NAME=elf32-tradlittlemips
BFD_ARCH=mips

ASFLAGS=-mips3

DEFS=-DARCH=$(ARCH) -DMACHINE=${MACHINE}
CFLAGS=$(DEFS) -mno-abicalls -mips3 -G 0 -nostdlib -fno-builtin -O2 
LFLAGS=-mips2 -M -no-check-sections

# It seems that on big endian either GCC or the simulators
# have the swl/swr/lwl/lwr instructions wrong. Just for sure,
# disable it with -mmemcpy (force calling memcpy instead of inlining)

ifeq (${MACHINE},indy)
 CFLAGS += -EB -mmemcpy -DBIG_ENDIAN -DHAVE_FPU -DFPU_LAZY
 BFD = elf32-big
endif

ifeq (${MACHINE},lgxemul)
 CFLAGS += -DHAVE_FPU -DFPU_LAZY
 BFD = ecoff-littlemips
endif

ifeq (${MACHINE},bgxemul)
 CFLAGS += -EB -mmemcpy -DBIG_ENDIAN -DHAVE_FPU -DFPU_LAZY
 BFD = ecoff-bigmips
endif

ifeq (${MACHINE},msim)
 BFD = binary
 CFLAGS += -msoft-float
endif

ifeq (${MACHINE},simics)
 BFD = elf32-little
 CFLAGS += -msoft-float
endif

../arch/$(ARCH)/_link.ld: ../arch/$(ARCH)/_link.ld.in
	$(CC) $(CFLAGS) -C -DBFD=${BFD} -E -x c $< | grep -v "^\#" > $@

arch_sources= \
	arch/start.S \
	arch/context.S \
	arch/panic.S \
	arch/mips.c \
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
	arch/fmath.c
