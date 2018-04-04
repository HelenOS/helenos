/*
 * Copyright (c) 2005 - 2006 Jakub Jermar
 * Copyright (c) 2006 Jakub Vana
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

/** @addtogroup ia64mm
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_PAGE_H_
#define KERN_ia64_PAGE_H_

#include <arch/mm/frame.h>

#ifndef __ASSEMBLER__
#include <assert.h>
#endif

#define PAGE_SIZE   FRAME_SIZE
#define PAGE_WIDTH  FRAME_WIDTH

/** Bit width of the TLB-locked portion of kernel address space. */
#define KERNEL_PAGE_WIDTH 	28	/* 256M */

#define PPN_SHIFT  12

#define VRN_SHIFT   61
#define VRN_MASK    (7ULL << VRN_SHIFT)
#define VA2VRN(va)  ((va) >> VRN_SHIFT)

#ifdef __ASSEMBLER__
#define VRN_KERNEL  7
#else
#define VRN_KERNEL  7ULL
#endif

#define REGION_REGISTERS  8

#define KA2PA(x)  (((uintptr_t) (x)) - (VRN_KERNEL << VRN_SHIFT))
#define PA2KA(x)  (((uintptr_t) (x)) + (VRN_KERNEL << VRN_SHIFT))

#define VHPT_WIDTH  20  /* 1M */
#define VHPT_SIZE   (1 << VHPT_WIDTH)

#define PTA_BASE_SHIFT  15

/** Memory Attributes. */
#define MA_WRITEBACK    0x00
#define MA_UNCACHEABLE  0x04

/** Privilege Levels. Only the most and the least privileged ones are ever used. */
#define PL_KERNEL  0x00
#define PL_USER    0x03

/* Access Rigths. Only certain combinations are used by the kernel. */
#define AR_READ     0x00
#define AR_EXECUTE  0x01
#define AR_WRITE    0x02

#ifndef __ASSEMBLER__

#include <arch/mm/as.h>
#include <arch/mm/frame.h>
#include <arch/interrupt.h>
#include <arch/barrier.h>
#include <arch/mm/asid.h>
#include <typedefs.h>
#include <debug.h>

struct vhpt_tag_info {
	unsigned long long tag : 63;
	unsigned int ti : 1;
} __attribute__((packed));

union vhpt_tag {
	struct vhpt_tag_info tag_info;
	unsigned tag_word;
};

struct vhpt_entry_present {
	/* Word 0 */
	unsigned int p : 1;
	unsigned int : 1;
	unsigned int ma : 3;
	unsigned int a : 1;
	unsigned int d : 1;
	unsigned int pl : 2;
	unsigned int ar : 3;
	unsigned long long ppn : 38;
	unsigned int : 2;
	unsigned int ed : 1;
	unsigned int ig1 : 11;

	/* Word 1 */
	unsigned int : 2;
	unsigned int ps : 6;
	unsigned int key : 24;
	unsigned int : 32;

	/* Word 2 */
	union vhpt_tag tag;

	/* Word 3 */
	uint64_t ig3 : 64;
} __attribute__((packed));

struct vhpt_entry_not_present {
	/* Word 0 */
	unsigned int p : 1;
	unsigned long long ig0 : 52;
	unsigned int ig1 : 11;

	/* Word 1 */
	unsigned int : 2;
	unsigned int ps : 6;
	unsigned long long ig2 : 56;

	/* Word 2 */
	union vhpt_tag tag;

	/* Word 3 */
	uint64_t ig3 : 64;
} __attribute__((packed));

typedef union {
	struct vhpt_entry_present present;
	struct vhpt_entry_not_present not_present;
	uint64_t word[4];
} vhpt_entry_t;

struct region_register_map {
	unsigned int ve : 1;
	unsigned int : 1;
	unsigned int ps : 6;
	unsigned int rid : 24;
	unsigned int : 32;
} __attribute__((packed));

typedef union {
	struct region_register_map map;
	unsigned long long word;
} region_register_t;

struct pta_register_map {
	unsigned int ve : 1;
	unsigned int : 1;
	unsigned int size : 6;
	unsigned int vf : 1;
	unsigned int : 6;
	unsigned long long base : 49;
} __attribute__((packed));

typedef union pta_register {
	struct pta_register_map map;
	uint64_t word;
} pta_register_t;

/** Return Translation Hashed Entry Address.
 *
 * VRN bits are used to read RID (ASID) from one
 * of the eight region registers registers.
 *
 * @param va Virtual address including VRN bits.
 *
 * @return Address of the head of VHPT collision chain.
 */
NO_TRACE static inline uint64_t thash(uint64_t va)
{
	uint64_t ret;

	asm volatile (
	    "thash %[ret] = %[va]\n"
	    : [ret] "=r" (ret)
	    : [va] "r" (va)
	);

	return ret;
}

/** Return Translation Hashed Entry Tag.
 *
 * VRN bits are used to read RID (ASID) from one
 * of the eight region registers.
 *
 * @param va Virtual address including VRN bits.
 *
 * @return The unique tag for VPN and RID in the collision chain returned by thash().
 */
NO_TRACE static inline uint64_t ttag(uint64_t va)
{
	uint64_t ret;

	asm volatile (
	    "ttag %[ret] = %[va]\n"
	    : [ret] "=r" (ret)
	    : [va] "r" (va)
	);

	return ret;
}

/** Read Region Register.
 *
 * @param i Region register index.
 *
 * @return Current contents of rr[i].
 */
NO_TRACE static inline uint64_t rr_read(size_t i)
{
	uint64_t ret;

	assert(i < REGION_REGISTERS);

	asm volatile (
	    "mov %[ret] = rr[%[index]]\n"
	    : [ret] "=r" (ret)
	    : [index] "r" (i << VRN_SHIFT)
	);

	return ret;
}

/** Write Region Register.
 *
 * @param i Region register index.
 * @param v Value to be written to rr[i].
 */
NO_TRACE static inline void rr_write(size_t i, uint64_t v)
{
	assert(i < REGION_REGISTERS);

	asm volatile (
	    "mov rr[%[index]] = %[value]\n"
	    :: [index] "r" (i << VRN_SHIFT),
	      [value] "r" (v)
	);
}

/** Read Page Table Register.
 *
 * @return Current value stored in PTA.
 */
NO_TRACE static inline uint64_t pta_read(void)
{
	uint64_t ret;

	asm volatile (
	    "mov %[ret] = cr.pta\n"
	    : [ret] "=r" (ret)
	);

	return ret;
}

/** Write Page Table Register.
 *
 * @param v New value to be stored in PTA.
 */
NO_TRACE static inline void pta_write(uint64_t v)
{
	asm volatile (
	    "mov cr.pta = %[value]\n"
	    :: [value] "r" (v)
	);
}

extern void page_arch_init(void);

extern vhpt_entry_t *vhpt_hash(uintptr_t page, asid_t asid);
extern bool vhpt_compare(uintptr_t page, asid_t asid, vhpt_entry_t *v);
extern void vhpt_set_record(vhpt_entry_t *v, uintptr_t page, asid_t asid, uintptr_t frame, int flags);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
