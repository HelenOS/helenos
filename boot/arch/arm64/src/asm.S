/*
 * Copyright (c) 2015 Petr Pavlu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <abi/asmtool.h>
#include <arch/arch.h>
#include <arch/regutils.h>

.section BOOTSTRAP

#define DIRECTORY_ENTRIES 16

/* MS-DOS stub */
msdos_stub:
	.ascii "MZ"                     /* MS-DOS signature */
	.space 0x3a                     /* Ignore fields up to byte at 0x3c */
	.long pe_header - msdos_stub    /* Offset to the PE header */

/* Portable Executable header */
pe_header:
	/* PE signature */
	.ascii "PE\x0\x0"

	/* COFF File Header */
	.short 0xaa64                   /* Machine = IMAGE_FILE_MACHINE_ARM64 */
	.short 1                        /* Number of sections */
	.long 0                         /* Time date stamp */
	.long 0                         /* Pointer to symbol table */
	.long 0                         /* Number of symbols */
	.short sec_table - opt_header   /* Size of optional header */
	/*
	 * Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE |
	 *   IMAGE_FILE_LARGE_ADDRESS_AWARE
	 */
	.short 0x22

	/* Optional header standard fields */
opt_header:
	.short 0x20b                    /* Magic = PE32+ */
	.byte 0                         /* Major linker version */
	.byte 0                         /* Minor linker version */
	.long payload_end - msdos_stub  /* Size of code */
	.long 0                         /* Size of initialized data */
	.long 0                         /* Size of uninitialized data */
	.long start - msdos_stub        /* Address of entry point */
	.long start - msdos_stub        /* Base of code */

	/* Optional header Windows-specific fields */
	.quad 0                         /* Image base */
	.long 4                         /* Section alignment */
	.long 4                         /* File alignment */
	.short 0                        /* Major operating system version */
	.short 0                        /* Minor operating system version */
	.short 0                        /* Major image version */
	.short 0                        /* Minor image version */
	.short 0                        /* Major subsystem version */
	.short 0                        /* Minor subsystem version */
	.long 0                         /* Win32 version value */
	.long payload_end - msdos_stub  /* Size of image */
	.long start - msdos_stub        /* Size of headers */
	.long 0                         /* Checksum */
	.short 10                       /* Subsystem = EFI application */
	.short 0                        /* DLL characteristics */
	.quad 0                         /* Size of stack reserve */
	.quad 0                         /* Size of stack commit */
	.quad 0                         /* Size of heap reserve */
	.quad 0                         /* Size of heap commit */
	.long 0                         /* Loader flags */
	.long DIRECTORY_ENTRIES         /* Number of RVA and sizes */
	.space DIRECTORY_ENTRIES * 8    /* Directory entries */

sec_table:
	.ascii ".text\x0\x0\x0"         /* Name */
	.long payload_end - start       /* Virtual size */
	.long start - msdos_stub        /* Virtual address */
	.long payload_end - start       /* Size of raw data */
	.long start - msdos_stub        /* Pointer to raw data */
	.long 0                         /* Pointer to relocations */
	.long 0                         /* Pointer to line numbers */
	.short 0                        /* Number of relocations */
	.short 0                        /* Number of line numbers */
	/*
	 * Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE |
	 *   IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE
	 */
	.long 0xe0000020

/** Boot loader entry point
 *
 * @param x0 UEFI image handle.
 * @param x1 Pointer to the UEFI system table.
 *
 */
SYMBOL(start)
	.hidden start

	/*
	 * Stay on the UEFI stack. Its size is at least 128 KiB, plenty for this
	 * boot loader.
	 */
	stp x29, x30, [sp, #-32]!
	mov x29, sp
	stp x0, x1, [sp, #16]

	/*
	 * Self-relocate the image. Pass a load address of the image (x0) and a
	 * pointer to the dynamic array (x1).
	 */
	adr x0, msdos_stub
	adrp x1, _DYNAMIC
	add x1, x1, #:lo12:_DYNAMIC
	bl self_relocate
	cbnz x0, __uefi_exit

	/*
	 * Flush the instruction cache of the relocated boot loader image.
	 */
	adr x0, msdos_stub
	adrp x1, payload_end
	sub x1, x1, x0
	bl smc_coherence

	/*
	 * Pass the image handle (x0), a pointer to the UEFI system table (x1),
	 * and the image load address (x2) to the boostrap function.
	 */
	ldp x0, x1, [sp, #16]
	adr x2, msdos_stub
	bl bootstrap

	__uefi_exit:
		ldp x29, x30, [sp], #32
		ret

FUNCTION_BEGIN(halt)
	.hidden halt

	b halt
FUNCTION_END(halt)

/** Flush instruction caches
 *
 * @param x0 Starting address of the flushing.
 * @param x1 Number of bytes to flush.
 *
 */
FUNCTION_BEGIN(smc_coherence)
	.hidden smc_coherence

	/* Initialize loop */
	mov x9, x0
	mov x10, xzr

	__dc_loop:
		/* Data or Unified Cache Line Clean */
		dc cvau, x9
		add x9, x9, #4
		add x10, x10, #4
		cmp x10, x1
		blo __dc_loop

	dsb ish

	/* Initialize loop */
	mov x9, x0
	mov x10, xzr

	__ic_loop:
		/* Instruction Cache Line Invalidate */
		ic ivau, x9
		add x9, x9, #4
		add x10, x10, #4
		cmp x10, x1
		blo __ic_loop

	dsb ish
	isb
	ret
FUNCTION_END(smc_coherence)

/** Flush data caches
 *
 * @param x0 Starting address of the flushing.
 * @param x1 Number of bytes to flush.
 *
 */
FUNCTION_BEGIN(dcache_flush)
	.hidden dcache_flush

	mov x9, x0
	mov x10, xzr

	__dc_flush_loop:
		/* Data or Unified Cache Line Clean */
		dc cvau, x9
		add x9, x9, #4
		add x10, x10, #4
		cmp x10, x1
		blo __dc_flush_loop

	dsb ish
	isb
	ret
FUNCTION_END(dcache_flush)

/** Kernel entry
 *
 * @param x0 Kernel entry point.
 * @param x1 Pointer to the bootinfo structure.
 *
 */
FUNCTION_BEGIN(jump_to_kernel)
	.hidden jump_to_kernel

	mrs x9, CurrentEL
	lsr x9, x9, 2

	cmp x9, #3
	b.eq __el3

	cmp x9, #2
	b.eq __el2

	cmp x9, #1
	b.eq __el1

	b halt

	__el3:
		msr sctlr_el2, xzr
		msr hcr_el2, xzr
		isb

		/* EL2 is AArch64, EL1 is Non-secure World */
		mov x9, #(1 << 10)
		orr x9, x9, #(1 << 0)
		msr scr_el3, x9
		isb

		/* EL2h */
		mov x9, #0x9
		msr spsr_el3, x9
		isb

		adr x9, __el2
		msr elr_el3, x9
		isb

		/* Switch to EL2 */
		eret

	__el2:
		msr sctlr_el1, xzr
		isb

		/* EL1 is AArch64 */
		mov x9, #(1 << 31)
		msr hcr_el2, x9
		isb

		/* EL1h */
		mov x9, #0x5
		msr spsr_el2, x9
		isb

		adr x9, __el1
		msr elr_el2, x9
		isb

		/* Switch to EL1 */
		eret

	__el1:
		/* Do not trap on FPU instructions */
		mrs x9, cpacr_el1
		orr x9, x9, #(3 << 20)
		msr cpacr_el1, x9
		dmb ish

		/* Disable MMU (removes the identity mapping provided by UEFI) */
		mrs x9, sctlr_el1
		bic x9, x9, #SCTLR_M_FLAG
		msr sctlr_el1, x9
		isb

		br x0
FUNCTION_END(jump_to_kernel)
