CC=gcc
AS=as
LD=ld

DEFS:=-DARCH=$(ARCH)

ifdef SMP
DEFS+=-D$(SMP)
endif

ifdef HT
DEFS+=-D$(HT)
endif

CPPFLAGS=$(DEFS) -nostdinc -I../include
CFLAGS=$(CPPFLAGS) -nostdlib -fno-builtin -fomit-frame-pointer -Werror-implicit-function-declaration -Wmissing-prototypes -Werror -O3
LFLAGS=-M -no-check-sections -T ../arch/ia32/_link.ld

arch_sources= \
	arch/context.s \
	arch/debug/panic.s \
	arch/cpuid.s \
	arch/delay.s \
	arch/asm.S \
	arch/proc/scheduler.c \
	arch/acpi/acpi.c \
	arch/acpi/madt.c \
	arch/bios/bios.c \
	arch/smp/ap.S \
	arch/smp/apic.c \
	arch/smp/mps.c \
	arch/smp/smp.c \
	arch/atomic.S \
	arch/smp/ipi.c \
	arch/ia32.c \
	arch/interrupt.c \
	arch/pm.c \
	arch/userspace.c \
	arch/cpu/cpu.c \
	arch/mm/frame.c \
	arch/mm/memory_init.c \
	arch/mm/page.c \
	arch/mm/tlb.c \
	arch/drivers/i8042.c \
	arch/drivers/i8254.c \
	arch/drivers/i8259.c \
	arch/drivers/ega.c \
	arch/boot/boot.S \
	arch/boot/memmap.S\
	arch/fpu_context.c
