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
#include <arch/mm/mmu.h>
#include <arch/mm/page.h>
#include <arch/asm.h>
#include <arch/barrier.h>
#include <arch/types.h>
#include <typedefs.h>

#define ITLB_ENTRY_COUNT		64
#define DTLB_ENTRY_COUNT		64

/** Page sizes. */
#define PAGESIZE_8K	0
#define PAGESIZE_64K	1
#define PAGESIZE_512K	2
#define PAGESIZE_4M	3

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
