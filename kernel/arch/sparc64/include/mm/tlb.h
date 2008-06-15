/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup sparc64mm	
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_TLB_H_
#define KERN_sparc64_TLB_H_

#define ITLB_ENTRY_COUNT		64
#define DTLB_ENTRY_COUNT		64

#define MEM_CONTEXT_KERNEL		0
#define MEM_CONTEXT_TEMP		1

/** Page sizes. */
#define PAGESIZE_8K	0
#define PAGESIZE_64K	1
#define PAGESIZE_512K	2
#define PAGESIZE_4M	3

/** Bit width of the TLB-locked portion of kernel address space. */
#define KERNEL_PAGE_WIDTH       22	/* 4M */

/* TLB Demap Operation types. */
#define TLB_DEMAP_PAGE		0
#define TLB_DEMAP_CONTEXT	1

#define TLB_DEMAP_TYPE_SHIFT	6

/* TLB Demap Operation Context register encodings. */
#define TLB_DEMAP_PRIMARY	0
#define TLB_DEMAP_SECONDARY	1
#define TLB_DEMAP_NUCLEUS	2

#define TLB_DEMAP_CONTEXT_SHIFT	4

/* TLB Tag Access shifts */
#define TLB_TAG_ACCESS_CONTEXT_SHIFT	0
#define TLB_TAG_ACCESS_CONTEXT_MASK	((1 << 13) - 1)
#define TLB_TAG_ACCESS_VPN_SHIFT	13

#ifndef __ASM__

#include <arch/mm/tte.h>
#include <arch/mm/mmu.h>
#include <arch/mm/page.h>
#include <arch/asm.h>
#include <arch/barrier.h>
#include <arch/types.h>

union tlb_context_reg {
	uint64_t v;
	struct {
		unsigned long : 51;
		unsigned context : 13;		/**< Context/ASID. */
	} __attribute__ ((packed));
};
typedef union tlb_context_reg tlb_context_reg_t;

/** I-/D-TLB Data In/Access Register type. */
typedef tte_data_t tlb_data_t;

/** I-/D-TLB Data Access Address in Alternate Space. */
union tlb_data_access_addr {
	uint64_t value;
	struct {
		uint64_t : 55;
		unsigned tlb_entry : 6;
		unsigned : 3;
	} __attribute__ ((packed));
};
typedef union tlb_data_access_addr tlb_data_access_addr_t;
typedef union tlb_data_access_addr tlb_tag_read_addr_t;

/** I-/D-TLB Tag Read Register. */
union tlb_tag_read_reg {
	uint64_t value;
	struct {
		uint64_t vpn : 51;	/**< Virtual Address bits 63:13. */
		unsigned context : 13;	/**< Context identifier. */
	} __attribute__ ((packed));
};
typedef union tlb_tag_read_reg tlb_tag_read_reg_t;
typedef union tlb_tag_read_reg tlb_tag_access_reg_t;


/** TLB Demap Operation Address. */
union tlb_demap_addr {
	uint64_t value;
	struct {
		uint64_t vpn: 51;	/**< Virtual Address bits 63:13. */
		unsigned : 6;		/**< Ignored. */
		unsigned type : 1;	/**< The type of demap operation. */
		unsigned context : 2;	/**< Context register selection. */
		unsigned : 4;		/**< Zero. */
	} __attribute__ ((packed));
};
typedef union tlb_demap_addr tlb_demap_addr_t;

/** TLB Synchronous Fault Status Register. */
union tlb_sfsr_reg {
	uint64_t value;
	struct {
		unsigned long : 40;	/**< Implementation dependent. */
		unsigned asi : 8;	/**< ASI. */
		unsigned : 2;
		unsigned ft : 7;	/**< Fault type. */
		unsigned e : 1;		/**< Side-effect bit. */
		unsigned ct : 2;	/**< Context Register selection. */
		unsigned pr : 1;	/**< Privilege bit. */
		unsigned w : 1;		/**< Write bit. */
		unsigned ow : 1;	/**< Overwrite bit. */
		unsigned fv : 1;	/**< Fault Valid bit. */
	} __attribute__ ((packed));
};
typedef union tlb_sfsr_reg tlb_sfsr_reg_t;

/** Read MMU Primary Context Register.
 *
 * @return Current value of Primary Context Register.
 */
static inline uint64_t mmu_primary_context_read(void)
{
	return asi_u64_read(ASI_DMMU, VA_PRIMARY_CONTEXT_REG);
}

/** Write MMU Primary Context Register.
 *
 * @param v New value of Primary Context Register.
 */
static inline void mmu_primary_context_write(uint64_t v)
{
	asi_u64_write(ASI_DMMU, VA_PRIMARY_CONTEXT_REG, v);
	flush_pipeline();
}

/** Read MMU Secondary Context Register.
 *
 * @return Current value of Secondary Context Register.
 */
static inline uint64_t mmu_secondary_context_read(void)
{
	return asi_u64_read(ASI_DMMU, VA_SECONDARY_CONTEXT_REG);
}

/** Write MMU Primary Context Register.
 *
 * @param v New value of Primary Context Register.
 */
static inline void mmu_secondary_context_write(uint64_t v)
{
	asi_u64_write(ASI_DMMU, VA_SECONDARY_CONTEXT_REG, v);
	flush_pipeline();
}

/** Read IMMU TLB Data Access Register.
 *
 * @param entry TLB Entry index.
 *
 * @return Current value of specified IMMU TLB Data Access Register.
 */
static inline uint64_t itlb_data_access_read(index_t entry)
{
	tlb_data_access_addr_t reg;
	
	reg.value = 0;
	reg.tlb_entry = entry;
	return asi_u64_read(ASI_ITLB_DATA_ACCESS_REG, reg.value);
}

/** Write IMMU TLB Data Access Register.
 *
 * @param entry TLB Entry index.
 * @param value Value to be written.
 */
static inline void itlb_data_access_write(index_t entry, uint64_t value)
{
	tlb_data_access_addr_t reg;
	
	reg.value = 0;
	reg.tlb_entry = entry;
	asi_u64_write(ASI_ITLB_DATA_ACCESS_REG, reg.value, value);
	flush_pipeline();
}

/** Read DMMU TLB Data Access Register.
 *
 * @param entry TLB Entry index.
 *
 * @return Current value of specified DMMU TLB Data Access Register.
 */
static inline uint64_t dtlb_data_access_read(index_t entry)
{
	tlb_data_access_addr_t reg;
	
	reg.value = 0;
	reg.tlb_entry = entry;
	return asi_u64_read(ASI_DTLB_DATA_ACCESS_REG, reg.value);
}

/** Write DMMU TLB Data Access Register.
 *
 * @param entry TLB Entry index.
 * @param value Value to be written.
 */
static inline void dtlb_data_access_write(index_t entry, uint64_t value)
{
	tlb_data_access_addr_t reg;
	
	reg.value = 0;
	reg.tlb_entry = entry;
	asi_u64_write(ASI_DTLB_DATA_ACCESS_REG, reg.value, value);
	membar();
}

/** Read IMMU TLB Tag Read Register.
 *
 * @param entry TLB Entry index.
 *
 * @return Current value of specified IMMU TLB Tag Read Register.
 */
static inline uint64_t itlb_tag_read_read(index_t entry)
{
	tlb_tag_read_addr_t tag;

	tag.value = 0;
	tag.tlb_entry =	entry;
	return asi_u64_read(ASI_ITLB_TAG_READ_REG, tag.value);
}

/** Read DMMU TLB Tag Read Register.
 *
 * @param entry TLB Entry index.
 *
 * @return Current value of specified DMMU TLB Tag Read Register.
 */
static inline uint64_t dtlb_tag_read_read(index_t entry)
{
	tlb_tag_read_addr_t tag;

	tag.value = 0;
	tag.tlb_entry =	entry;
	return asi_u64_read(ASI_DTLB_TAG_READ_REG, tag.value);
}

/** Write IMMU TLB Tag Access Register.
 *
 * @param v Value to be written.
 */
static inline void itlb_tag_access_write(uint64_t v)
{
	asi_u64_write(ASI_IMMU, VA_IMMU_TAG_ACCESS, v);
	flush_pipeline();
}

/** Read IMMU TLB Tag Access Register.
 *
 * @return Current value of IMMU TLB Tag Access Register.
 */
static inline uint64_t itlb_tag_access_read(void)
{
	return asi_u64_read(ASI_IMMU, VA_IMMU_TAG_ACCESS);
}

/** Write DMMU TLB Tag Access Register.
 *
 * @param v Value to be written.
 */
static inline void dtlb_tag_access_write(uint64_t v)
{
	asi_u64_write(ASI_DMMU, VA_DMMU_TAG_ACCESS, v);
	membar();
}

/** Read DMMU TLB Tag Access Register.
 *
 * @return Current value of DMMU TLB Tag Access Register.
 */
static inline uint64_t dtlb_tag_access_read(void)
{
	return asi_u64_read(ASI_DMMU, VA_DMMU_TAG_ACCESS);
}


/** Write IMMU TLB Data in Register.
 *
 * @param v Value to be written.
 */
static inline void itlb_data_in_write(uint64_t v)
{
	asi_u64_write(ASI_ITLB_DATA_IN_REG, 0, v);
	flush_pipeline();
}

/** Write DMMU TLB Data in Register.
 *
 * @param v Value to be written.
 */
static inline void dtlb_data_in_write(uint64_t v)
{
	asi_u64_write(ASI_DTLB_DATA_IN_REG, 0, v);
	membar();
}

/** Read ITLB Synchronous Fault Status Register.
 *
 * @return Current content of I-SFSR register.
 */
static inline uint64_t itlb_sfsr_read(void)
{
	return asi_u64_read(ASI_IMMU, VA_IMMU_SFSR);
}

/** Write ITLB Synchronous Fault Status Register.
 *
 * @param v New value of I-SFSR register.
 */
static inline void itlb_sfsr_write(uint64_t v)
{
	asi_u64_write(ASI_IMMU, VA_IMMU_SFSR, v);
	flush_pipeline();
}

/** Read DTLB Synchronous Fault Status Register.
 *
 * @return Current content of D-SFSR register.
 */
static inline uint64_t dtlb_sfsr_read(void)
{
	return asi_u64_read(ASI_DMMU, VA_DMMU_SFSR);
}

/** Write DTLB Synchronous Fault Status Register.
 *
 * @param v New value of D-SFSR register.
 */
static inline void dtlb_sfsr_write(uint64_t v)
{
	asi_u64_write(ASI_DMMU, VA_DMMU_SFSR, v);
	membar();
}

/** Read DTLB Synchronous Fault Address Register.
 *
 * @return Current content of D-SFAR register.
 */
static inline uint64_t dtlb_sfar_read(void)
{
	return asi_u64_read(ASI_DMMU, VA_DMMU_SFAR);
}

/** Perform IMMU TLB Demap Operation.
 *
 * @param type Selects between context and page demap.
 * @param context_encoding Specifies which Context register has Context ID for
 * 	demap.
 * @param page Address which is on the page to be demapped.
 */
static inline void itlb_demap(int type, int context_encoding, uintptr_t page)
{
	tlb_demap_addr_t da;
	page_address_t pg;
	
	da.value = 0;
	pg.address = page;
	
	da.type = type;
	da.context = context_encoding;
	da.vpn = pg.vpn;
	
	asi_u64_write(ASI_IMMU_DEMAP, da.value, 0);	/* da.value is the
							 * address within the
							 * ASI */ 
	flush_pipeline();
}

/** Perform DMMU TLB Demap Operation.
 *
 * @param type Selects between context and page demap.
 * @param context_encoding Specifies which Context register has Context ID for
 *	 demap.
 * @param page Address which is on the page to be demapped.
 */
static inline void dtlb_demap(int type, int context_encoding, uintptr_t page)
{
	tlb_demap_addr_t da;
	page_address_t pg;
	
	da.value = 0;
	pg.address = page;
	
	da.type = type;
	da.context = context_encoding;
	da.vpn = pg.vpn;
	
	asi_u64_write(ASI_DMMU_DEMAP, da.value, 0);	/* da.value is the
							 * address within the
							 * ASI */ 
	membar();
}

extern void fast_instruction_access_mmu_miss(unative_t unused, istate_t *istate);
extern void fast_data_access_mmu_miss(tlb_tag_access_reg_t tag, istate_t *istate);
extern void fast_data_access_protection(tlb_tag_access_reg_t tag , istate_t *istate);

extern void dtlb_insert_mapping(uintptr_t page, uintptr_t frame, int pagesize, bool locked, bool cacheable);

extern void dump_sfsr_and_sfar(void);

#endif /* !def __ASM__ */

#endif

/** @}
 */
