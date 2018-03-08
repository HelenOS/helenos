/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup amd64
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_PM_H_
#define KERN_amd64_PM_H_

#ifndef __ASSEMBLER__
	#include <typedefs.h>
	#include <arch/context.h>
#endif

#define IDT_ITEMS  64
#define GDT_ITEMS  8


#define NULL_DES     0
/* Warning: Do not reorder the following items, unless you look into syscall.c! */
#define KTEXT_DES    1
#define KDATA_DES    2
#define UDATA_DES    3
#define UTEXT_DES    4
#define KTEXT32_DES  5
/* End of warning */
#define TSS_DES      6

#ifdef CONFIG_FB

#define VESA_INIT_SEGMENT  0x8000
#define VESA_INIT_CODE_DES      8
#define VESA_INIT_DATA_DES      9

#undef GDT_ITEMS
#define GDT_ITEMS  10

#endif /* CONFIG_FB */

#define GDT_SELECTOR(des)  ((des) << 3)

#define PL_KERNEL  0
#define PL_USER    3

#define AR_PRESENT    (1 << 7)
#define AR_DATA       (2 << 3)
#define AR_CODE       (3 << 3)
#define AR_WRITABLE   (1 << 1)
#define AR_READABLE   (1 << 1)
#define AR_TSS        (0x09U)
#define AR_INTERRUPT  (0x0eU)
#define AR_TRAP       (0x0fU)

#define DPL_KERNEL  (PL_KERNEL << 5)
#define DPL_USER    (PL_USER << 5)

#define TSS_BASIC_SIZE  104
#define TSS_IOMAP_SIZE  (8 * 1024 + 1)  /* 8K for bitmap + 1 terminating byte for convenience */

#define IO_PORTS  (64 * 1024)

#ifndef __ASSEMBLER__

typedef struct {
	unsigned limit_0_15: 16;
	unsigned base_0_15: 16;
	unsigned base_16_23: 8;
	unsigned access: 8;
	unsigned limit_16_19: 4;
	unsigned available: 1;
	unsigned longmode: 1;
	unsigned special: 1;
	unsigned granularity : 1;
	unsigned base_24_31: 8;
} __attribute__ ((packed)) descriptor_t;

typedef struct {
	unsigned limit_0_15: 16;
	unsigned base_0_15: 16;
	unsigned base_16_23: 8;
	unsigned type: 4;
	unsigned : 1;
	unsigned dpl : 2;
	unsigned present : 1;
	unsigned limit_16_19: 4;
	unsigned available: 1;
	unsigned : 2;
	unsigned granularity : 1;
	unsigned base_24_31: 8;
	unsigned base_32_63 : 32;
	unsigned  : 32;
} __attribute__ ((packed)) tss_descriptor_t;

typedef struct {
	unsigned offset_0_15: 16;
	unsigned selector: 16;
	unsigned ist:3;
	unsigned unused: 5;
	unsigned type: 5;
	unsigned dpl: 2;
	unsigned present : 1;
	unsigned offset_16_31: 16;
	unsigned offset_32_63: 32;
	unsigned  : 32;
} __attribute__ ((packed)) idescriptor_t;

typedef struct {
	uint16_t limit;
	uint64_t base;
} __attribute__ ((packed)) ptr_16_64_t;

typedef struct {
	uint16_t limit;
	uint32_t base;
} __attribute__ ((packed)) ptr_16_32_t;

typedef struct {
	uint32_t reserve1;
	uint64_t rsp0;
	uint64_t rsp1;
	uint64_t rsp2;
	uint64_t reserve2;
	uint64_t ist1;
	uint64_t ist2;
	uint64_t ist3;
	uint64_t ist4;
	uint64_t ist5;
	uint64_t ist6;
	uint64_t ist7;
	uint64_t reserve3;
	uint16_t reserve4;
	uint16_t iomap_base;
	uint8_t iomap[TSS_IOMAP_SIZE];
} __attribute__ ((packed)) tss_t;

extern tss_t *tss_p;

extern descriptor_t gdt[];
extern idescriptor_t idt[];

extern ptr_16_64_t gdtr;
extern ptr_16_32_t protected_ap_gdtr;

extern void pm_init(void);

extern void gdt_tss_setbase(descriptor_t *d, uintptr_t base);
extern void gdt_tss_setlimit(descriptor_t *d, uint32_t limit);

extern void idt_init(void);
extern void idt_setoffset(idescriptor_t *d, uintptr_t offset);

extern void tss_initialize(tss_t *t);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
