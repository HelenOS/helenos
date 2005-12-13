/*
 * Copyright (C) 2005 Jakub Jermar
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

#ifndef __sparc64_TLB_H__
#define __sparc64_TLB_H__

#include <arch/mm/tte.h>
#include <arch/mm/page.h>
#include <arch/asm.h>
#include <arch/barrier.h>
#include <arch/types.h>
#include <typedefs.h>

#define ITLB_ENTRY_COUNT		64
#define DTLB_ENTRY_COUNT		64

/** I-MMU ASIs. */
#define ASI_IMMU			0x50
#define ASI_IMMU_TSB_8KB_PTR_REG	0x51	
#define ASI_IMMU_TSB_64KB_PTR_REG	0x52
#define ASI_ITLB_DATA_IN_REG		0x54
#define ASI_ITLB_DATA_ACCESS_REG	0x55
#define ASI_ITLB_TAG_READ_REG		0x56
#define ASI_IMMU_DEMAP			0x57

/** Virtual Addresses within ASI_IMMU. */
#define VA_IMMU_TAG_TARGET		0x0	/**< IMMU tag target register. */
#define VA_IMMU_SFSR			0x18	/**< IMMU sync fault status register. */
#define VA_IMMU_TSB_BASE		0x28	/**< IMMU TSB base register. */
#define VA_IMMU_TAG_ACCESS		0x30	/**< IMMU TLB tag access register. */

/** D-MMU ASIs. */
#define ASI_DMMU			0x58
#define ASI_DMMU_TSB_8KB_PTR_REG	0x59	
#define ASI_DMMU_TSB_64KB_PTR_REG	0x5a
#define ASI_DMMU_TSB_DIRECT_PTR_REG	0x5b
#define ASI_DTLB_DATA_IN_REG		0x5c
#define ASI_DTLB_DATA_ACCESS_REG	0x5d
#define ASI_DTLB_TAG_READ_REG		0x5e
#define ASI_DMMU_DEMAP			0x5f

/** Virtual Addresses within ASI_DMMU. */
#define VA_DMMU_TAG_TARGET		0x0	/**< DMMU tag target register. */
#define VA_PRIMARY_CONTEXT_REG		0x8	/**< DMMU primary context register. */
#define VA_SECONDARY_CONTEXT_REG	0x10	/**< DMMU secondary context register. */
#define VA_DMMU_SFSR			0x18	/**< DMMU sync fault status register. */
#define VA_DMMU_SFAR			0x20	/**< DMMU sync fault address register. */
#define VA_DMMU_TSB_BASE		0x28	/**< DMMU TSB base register. */
#define VA_DMMU_TAG_ACCESS		0x30	/**< DMMU TLB tag access register. */
#define VA_DMMU_VA_WATCHPOINT_REG	0x38	/**< DMMU VA data watchpoint register. */
#define VA_DMMU_PA_WATCHPOINT_REG	0x40	/**< DMMU PA data watchpoint register. */

/** I-/D-TLB Data In/Access Register type. */
typedef tte_data_t tlb_data_t;

/** I-/D-TLB Data Access Address in Alternate Space. */
union tlb_data_access_addr {
	__u64 value;
	struct {
		__u64 : 55;
		unsigned tlb_entry : 6;
		unsigned : 3;
	} __attribute__ ((packed));
};
typedef union tlb_data_access_addr tlb_data_access_addr_t;
typedef union tlb_data_access_addr tlb_tag_read_addr_t;

/** I-/D-TLB Tag Read Register. */
union tlb_tag_read_reg {
	__u64 value;
	struct {
		__u64 vpn : 51;		/**< Virtual Address bits 63:13. */
		unsigned context : 13;	/**< Context identifier. */
	} __attribute__ ((packed));
};
typedef union tlb_tag_read_reg tlb_tag_read_reg_t;
typedef union tlb_tag_read_reg tlb_tag_access_reg_t;

/** TLB Demap Operation types. */
#define TLB_DEMAP_PAGE		0
#define TLB_DEMAP_CONTEXT	1

/** TLB Demap Operation Context register encodings. */
#define TLB_DEMAP_PRIMARY	0
#define TLB_DEMAP_SECONDARY	1
#define TLB_DEMAP_NUCLEUS	2

/** TLB Demap Operation Address. */
union tlb_demap_addr {
	__u64 value;
	struct {
		__u64 vpn: 51;		/**< Virtual Address bits 63:13. */
		unsigned : 6;		/**< Ignored. */
		unsigned type : 1;	/**< The type of demap operation. */
		unsigned context : 2;	/**< Context register selection. */
		unsigned : 4;		/**< Zero. */
	} __attribute__ ((packed));
};
typedef union tlb_demap_addr tlb_demap_addr_t;

/** Read IMMU TLB Data Access Register.
 *
 * @param entry TLB Entry index.
 *
 * @return Current value of specified IMMU TLB Data Access Register.
 */
static inline __u64 itlb_data_access_read(index_t entry)
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
static inline __u64 itlb_data_access_write(index_t entry, __u64 value)
{
	tlb_data_access_addr_t reg;
	
	reg.value = 0;
	reg.tlb_entry = entry;
	asi_u64_write(ASI_ITLB_DATA_ACCESS_REG, reg.value, value);
	flush();
}

/** Read DMMU TLB Data Access Register.
 *
 * @param entry TLB Entry index.
 *
 * @return Current value of specified DMMU TLB Data Access Register.
 */
static inline __u64 dtlb_data_access_read(index_t entry)
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
static inline __u64 dtlb_data_access_write(index_t entry, __u64 value)
{
	tlb_data_access_addr_t reg;
	
	reg.value = 0;
	reg.tlb_entry = entry;
	asi_u64_write(ASI_DTLB_DATA_ACCESS_REG, reg.value, value);
	flush();
}

/** Read IMMU TLB Tag Read Register.
 *
 * @param entry TLB Entry index.
 *
 * @return Current value of specified IMMU TLB Tag Read Register.
 */
static inline __u64 itlb_tag_read_read(index_t entry)
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
static inline __u64 dtlb_tag_read_read(index_t entry)
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
static inline void itlb_tag_access_write(__u64 v)
{
	asi_u64_write(ASI_IMMU, VA_IMMU_TAG_ACCESS, v);
	flush();
}

/** Write DMMU TLB Tag Access Register.
 *
 * @param v Value to be written.
 */
static inline void dtlb_tag_access_write(__u64 v)
{
	asi_u64_write(ASI_DMMU, VA_DMMU_TAG_ACCESS, v);
	flush();
}

/** Write IMMU TLB Data in Register.
 *
 * @param v Value to be written.
 */
static inline void itlb_data_in_write(__u64 v)
{
	asi_u64_write(ASI_ITLB_DATA_IN_REG, 0, v);
	flush();
}

/** Write DMMU TLB Data in Register.
 *
 * @param v Value to be written.
 */
static inline void dtlb_data_in_write(__u64 v)
{
	asi_u64_write(ASI_DTLB_DATA_IN_REG, 0, v);
	flush();
}

/** Perform IMMU TLB Demap Operation.
 *
 * @param type Selects between context and page demap.
 * @param context_encoding Specifies which Context register has Context ID for demap.
 * @param page Address which is on the page to be demapped.
 */
static inline void itlb_demap(int type, int context_encoding, __address page)
{
	tlb_demap_addr_t da;
	page_address_t pg;
	
	da.value = 0;
	pg.address = page;
	
	da.type = type;
	da.context = context_encoding;
	da.vpn = pg.vpn;
	
	asi_u64_write(ASI_IMMU_DEMAP, da.value, 0);
	flush();
}

/** Perform DMMU TLB Demap Operation.
 *
 * @param type Selects between context and page demap.
 * @param context_encoding Specifies which Context register has Context ID for demap.
 * @param page Address which is on the page to be demapped.
 */
static inline void dtlb_demap(int type, int context_encoding, __address page)
{
	tlb_demap_addr_t da;
	page_address_t pg;
	
	da.value = 0;
	pg.address = page;
	
	da.type = type;
	da.context = context_encoding;
	da.vpn = pg.vpn;
	
	asi_u64_write(ASI_DMMU_DEMAP, da.value, 0);
	flush();
}

#endif
