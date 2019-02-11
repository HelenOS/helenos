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

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_TLB_sun4u_H_
#define KERN_sparc64_TLB_sun4u_H_

#if defined (US)
#define ITLB_ENTRY_COUNT		64
#define DTLB_ENTRY_COUNT		64
#define DTLB_MAX_LOCKED_ENTRIES		DTLB_ENTRY_COUNT
#endif

/** TLB_DSMALL is the only of the three DMMUs that can hold locked entries. */
#if defined (US3)
#define DTLB_MAX_LOCKED_ENTRIES		16
#endif

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
#if defined (US3)
#define TLB_DEMAP_ALL		2
#endif

#define TLB_DEMAP_TYPE_SHIFT	6

/* TLB Demap Operation Context register encodings. */
#define TLB_DEMAP_PRIMARY	0
#define TLB_DEMAP_SECONDARY	1
#define TLB_DEMAP_NUCLEUS	2

/* There are more TLBs in one MMU in US3, their codes are defined here. */
#if defined (US3)
/* D-MMU: one small (16-entry) TLB and two big (512-entry) TLBs */
#define TLB_DSMALL	0
#define TLB_DBIG_0	2
#define TLB_DBIG_1	3

/* I-MMU: one small (16-entry) TLB and one big TLB */
#define TLB_ISMALL	0
#define TLB_IBIG	2
#endif

#define TLB_DEMAP_CONTEXT_SHIFT	4

/* TLB Tag Access shifts */
#define TLB_TAG_ACCESS_CONTEXT_SHIFT	0
#define TLB_TAG_ACCESS_CONTEXT_MASK	((1 << 13) - 1)
#define TLB_TAG_ACCESS_VPN_SHIFT	13

#ifndef __ASSEMBLER__

#include <arch/mm/tte.h>
#include <arch/mm/mmu.h>
#include <arch/mm/page.h>
#include <arch/asm.h>
#include <arch/barrier.h>
#include <barrier.h>
#include <typedefs.h>
#include <trace.h>
#include <arch/register.h>
#include <arch/cpu.h>

union tlb_context_reg {
	uint64_t v;
	struct {
		unsigned long : 51;
		unsigned context : 13;		/**< Context/ASID. */
	} __attribute__((packed));
};
typedef union tlb_context_reg tlb_context_reg_t;

/** I-/D-TLB Data In/Access Register type. */
typedef tte_data_t tlb_data_t;

/** I-/D-TLB Data Access Address in Alternate Space. */

#if defined (US)

union tlb_data_access_addr {
	uint64_t value;
	struct {
		uint64_t : 55;
		unsigned tlb_entry : 6;
		unsigned : 3;
	} __attribute__((packed));
};
typedef union tlb_data_access_addr dtlb_data_access_addr_t;
typedef union tlb_data_access_addr dtlb_tag_read_addr_t;
typedef union tlb_data_access_addr itlb_data_access_addr_t;
typedef union tlb_data_access_addr itlb_tag_read_addr_t;

#elif defined (US3)

/*
 * In US3, I-MMU and D-MMU have different formats of the data
 * access register virtual address. In the corresponding
 * structures the member variable for the entry number is
 * called "local_tlb_entry" - it contrasts with the "tlb_entry"
 * for the US data access register VA structure. The rationale
 * behind this is to prevent careless mistakes in the code
 * caused by setting only the entry number and not the TLB
 * number in the US3 code (when taking the code from US).
 */

union dtlb_data_access_addr {
	uint64_t value;
	struct {
		uint64_t : 45;
		unsigned : 1;
		unsigned tlb_number : 2;
		unsigned : 4;
		unsigned local_tlb_entry : 9;
		unsigned : 3;
	} __attribute__((packed));
};
typedef union dtlb_data_access_addr dtlb_data_access_addr_t;
typedef union dtlb_data_access_addr dtlb_tag_read_addr_t;

union itlb_data_access_addr {
	uint64_t value;
	struct {
		uint64_t : 45;
		unsigned : 1;
		unsigned tlb_number : 2;
		unsigned : 6;
		unsigned local_tlb_entry : 7;
		unsigned : 3;
	} __attribute__((packed));
};
typedef union itlb_data_access_addr itlb_data_access_addr_t;
typedef union itlb_data_access_addr itlb_tag_read_addr_t;

#endif

/** I-/D-TLB Tag Read Register. */
union tlb_tag_read_reg {
	uint64_t value;
	struct {
		uint64_t vpn : 51;	/**< Virtual Address bits 63:13. */
		unsigned context : 13;	/**< Context identifier. */
	} __attribute__((packed));
};
typedef union tlb_tag_read_reg tlb_tag_read_reg_t;
typedef union tlb_tag_read_reg tlb_tag_access_reg_t;

/** TLB Demap Operation Address. */
union tlb_demap_addr {
	uint64_t value;
	struct {
		uint64_t vpn : 51;	/**< Virtual Address bits 63:13. */
#if defined (US)
		unsigned : 6;		/**< Ignored. */
		unsigned type : 1;	/**< The type of demap operation. */
#elif defined (US3)
		unsigned : 5;		/**< Ignored. */
		unsigned type : 2;	/**< The type of demap operation. */
#endif
		unsigned context : 2;	/**< Context register selection. */
		unsigned : 4;		/**< Zero. */
	} __attribute__((packed));
};
typedef union tlb_demap_addr tlb_demap_addr_t;

/** TLB Synchronous Fault Status Register. */
union tlb_sfsr_reg {
	uint64_t value;
	struct {
#if defined (US)
		unsigned long : 40;	/**< Implementation dependent. */
		unsigned asi : 8;	/**< ASI. */
		unsigned : 2;
		unsigned ft : 7;	/**< Fault type. */
#elif defined (US3)
		unsigned long : 39;	/**< Implementation dependent. */
		unsigned nf : 1;	/**< Non-faulting load. */
		unsigned asi : 8;	/**< ASI. */
		unsigned tm : 1;	/**< I-TLB miss. */
		unsigned : 3;		/**< Reserved. */
		unsigned ft : 5;	/**< Fault type. */
#endif
		unsigned e : 1;		/**< Side-effect bit. */
		unsigned ct : 2;	/**< Context Register selection. */
		unsigned pr : 1;	/**< Privilege bit. */
		unsigned w : 1;		/**< Write bit. */
		unsigned ow : 1;	/**< Overwrite bit. */
		unsigned fv : 1;	/**< Fault Valid bit. */
	} __attribute__((packed));
};
typedef union tlb_sfsr_reg tlb_sfsr_reg_t;

#if defined (US3)

/*
 * Functions for determining the number of entries in TLBs. They either return
 * a constant value or a value based on the CPU autodetection.
 */

/**
 * Determine the number of entries in the DMMU's small TLB.
 */
_NO_TRACE static inline uint16_t tlb_dsmall_size(void)
{
	return 16;
}

/**
 * Determine the number of entries in each DMMU's big TLB.
 */
_NO_TRACE static inline uint16_t tlb_dbig_size(void)
{
	return 512;
}

/**
 * Determine the number of entries in the IMMU's small TLB.
 */
_NO_TRACE static inline uint16_t tlb_ismall_size(void)
{
	return 16;
}

/**
 * Determine the number of entries in the IMMU's big TLB.
 */
_NO_TRACE static inline uint16_t tlb_ibig_size(void)
{
	if (((ver_reg_t) ver_read()).impl == IMPL_ULTRASPARCIV_PLUS)
		return 512;
	else
		return 128;
}

#endif

/** Read MMU Primary Context Register.
 *
 * @return		Current value of Primary Context Register.
 */
_NO_TRACE static inline uint64_t mmu_primary_context_read(void)
{
	return asi_u64_read(ASI_DMMU, VA_PRIMARY_CONTEXT_REG);
}

/** Write MMU Primary Context Register.
 *
 * @param v		New value of Primary Context Register.
 */
_NO_TRACE static inline void mmu_primary_context_write(uint64_t v)
{
	asi_u64_write(ASI_DMMU, VA_PRIMARY_CONTEXT_REG, v);
	flush_pipeline();
}

/** Read MMU Secondary Context Register.
 *
 * @return		Current value of Secondary Context Register.
 */
_NO_TRACE static inline uint64_t mmu_secondary_context_read(void)
{
	return asi_u64_read(ASI_DMMU, VA_SECONDARY_CONTEXT_REG);
}

/** Write MMU Primary Context Register.
 *
 * @param v		New value of Primary Context Register.
 */
_NO_TRACE static inline void mmu_secondary_context_write(uint64_t v)
{
	asi_u64_write(ASI_DMMU, VA_SECONDARY_CONTEXT_REG, v);
	flush_pipeline();
}

#if defined (US)

/** Read IMMU TLB Data Access Register.
 *
 * @param entry		TLB Entry index.
 *
 * @return		Current value of specified IMMU TLB Data Access
 * 			Register.
 */
_NO_TRACE static inline uint64_t itlb_data_access_read(size_t entry)
{
	itlb_data_access_addr_t reg;

	reg.value = 0;
	reg.tlb_entry = entry;
	return asi_u64_read(ASI_ITLB_DATA_ACCESS_REG, reg.value);
}

/** Write IMMU TLB Data Access Register.
 *
 * @param entry		TLB Entry index.
 * @param value		Value to be written.
 */
_NO_TRACE static inline void itlb_data_access_write(size_t entry, uint64_t value)
{
	itlb_data_access_addr_t reg;

	reg.value = 0;
	reg.tlb_entry = entry;
	asi_u64_write(ASI_ITLB_DATA_ACCESS_REG, reg.value, value);
	flush_pipeline();
}

/** Read DMMU TLB Data Access Register.
 *
 * @param entry		TLB Entry index.
 *
 * @return		Current value of specified DMMU TLB Data Access
 * 			Register.
 */
_NO_TRACE static inline uint64_t dtlb_data_access_read(size_t entry)
{
	dtlb_data_access_addr_t reg;

	reg.value = 0;
	reg.tlb_entry = entry;
	return asi_u64_read(ASI_DTLB_DATA_ACCESS_REG, reg.value);
}

/** Write DMMU TLB Data Access Register.
 *
 * @param entry		TLB Entry index.
 * @param value		Value to be written.
 */
_NO_TRACE static inline void dtlb_data_access_write(size_t entry, uint64_t value)
{
	dtlb_data_access_addr_t reg;

	reg.value = 0;
	reg.tlb_entry = entry;
	asi_u64_write(ASI_DTLB_DATA_ACCESS_REG, reg.value, value);
	membar();
}

/** Read IMMU TLB Tag Read Register.
 *
 * @param entry		TLB Entry index.
 *
 * @return		Current value of specified IMMU TLB Tag Read Register.
 */
_NO_TRACE static inline uint64_t itlb_tag_read_read(size_t entry)
{
	itlb_tag_read_addr_t tag;

	tag.value = 0;
	tag.tlb_entry =	entry;
	return asi_u64_read(ASI_ITLB_TAG_READ_REG, tag.value);
}

/** Read DMMU TLB Tag Read Register.
 *
 * @param entry		TLB Entry index.
 *
 * @return		Current value of specified DMMU TLB Tag Read Register.
 */
_NO_TRACE static inline uint64_t dtlb_tag_read_read(size_t entry)
{
	dtlb_tag_read_addr_t tag;

	tag.value = 0;
	tag.tlb_entry =	entry;
	return asi_u64_read(ASI_DTLB_TAG_READ_REG, tag.value);
}

#elif defined (US3)

/** Read IMMU TLB Data Access Register.
 *
 * @param tlb		TLB number (one of TLB_ISMALL or TLB_IBIG)
 * @param entry		TLB Entry index.
 *
 * @return		Current value of specified IMMU TLB Data Access
 * 			Register.
 */
_NO_TRACE static inline uint64_t itlb_data_access_read(int tlb, size_t entry)
{
	itlb_data_access_addr_t reg;

	reg.value = 0;
	reg.tlb_number = tlb;
	reg.local_tlb_entry = entry;
	return asi_u64_read(ASI_ITLB_DATA_ACCESS_REG, reg.value);
}

/** Write IMMU TLB Data Access Register.
 * @param tlb		TLB number (one of TLB_ISMALL or TLB_IBIG)
 * @param entry		TLB Entry index.
 * @param value		Value to be written.
 */
_NO_TRACE static inline void itlb_data_access_write(int tlb, size_t entry,
    uint64_t value)
{
	itlb_data_access_addr_t reg;

	reg.value = 0;
	reg.tlb_number = tlb;
	reg.local_tlb_entry = entry;
	asi_u64_write(ASI_ITLB_DATA_ACCESS_REG, reg.value, value);
	flush_pipeline();
}

/** Read DMMU TLB Data Access Register.
 *
 * @param tlb		TLB number (one of TLB_DSMALL, TLB_DBIG, TLB_DBIG)
 * @param entry		TLB Entry index.
 *
 * @return		Current value of specified DMMU TLB Data Access
 * 			Register.
 */
_NO_TRACE static inline uint64_t dtlb_data_access_read(int tlb, size_t entry)
{
	dtlb_data_access_addr_t reg;

	reg.value = 0;
	reg.tlb_number = tlb;
	reg.local_tlb_entry = entry;
	return asi_u64_read(ASI_DTLB_DATA_ACCESS_REG, reg.value);
}

/** Write DMMU TLB Data Access Register.
 *
 * @param tlb		TLB number (one of TLB_DSMALL, TLB_DBIG_0, TLB_DBIG_1)
 * @param entry		TLB Entry index.
 * @param value		Value to be written.
 */
_NO_TRACE static inline void dtlb_data_access_write(int tlb, size_t entry,
    uint64_t value)
{
	dtlb_data_access_addr_t reg;

	reg.value = 0;
	reg.tlb_number = tlb;
	reg.local_tlb_entry = entry;
	asi_u64_write(ASI_DTLB_DATA_ACCESS_REG, reg.value, value);
	membar();
}

/** Read IMMU TLB Tag Read Register.
 *
 * @param tlb		TLB number (one of TLB_ISMALL or TLB_IBIG)
 * @param entry		TLB Entry index.
 *
 * @return		Current value of specified IMMU TLB Tag Read Register.
 */
_NO_TRACE static inline uint64_t itlb_tag_read_read(int tlb, size_t entry)
{
	itlb_tag_read_addr_t tag;

	tag.value = 0;
	tag.tlb_number = tlb;
	tag.local_tlb_entry = entry;
	return asi_u64_read(ASI_ITLB_TAG_READ_REG, tag.value);
}

/** Read DMMU TLB Tag Read Register.
 *
 * @param tlb		TLB number (one of TLB_DSMALL, TLB_DBIG_0, TLB_DBIG_1)
 * @param entry		TLB Entry index.
 *
 * @return		Current value of specified DMMU TLB Tag Read Register.
 */
_NO_TRACE static inline uint64_t dtlb_tag_read_read(int tlb, size_t entry)
{
	dtlb_tag_read_addr_t tag;

	tag.value = 0;
	tag.tlb_number = tlb;
	tag.local_tlb_entry = entry;
	return asi_u64_read(ASI_DTLB_TAG_READ_REG, tag.value);
}

#endif

/** Write IMMU TLB Tag Access Register.
 *
 * @param v		Value to be written.
 */
_NO_TRACE static inline void itlb_tag_access_write(uint64_t v)
{
	asi_u64_write(ASI_IMMU, VA_IMMU_TAG_ACCESS, v);
	flush_pipeline();
}

/** Read IMMU TLB Tag Access Register.
 *
 * @return		Current value of IMMU TLB Tag Access Register.
 */
_NO_TRACE static inline uint64_t itlb_tag_access_read(void)
{
	return asi_u64_read(ASI_IMMU, VA_IMMU_TAG_ACCESS);
}

/** Write DMMU TLB Tag Access Register.
 *
 * @param v		Value to be written.
 */
_NO_TRACE static inline void dtlb_tag_access_write(uint64_t v)
{
	asi_u64_write(ASI_DMMU, VA_DMMU_TAG_ACCESS, v);
	membar();
}

/** Read DMMU TLB Tag Access Register.
 *
 * @return 		Current value of DMMU TLB Tag Access Register.
 */
_NO_TRACE static inline uint64_t dtlb_tag_access_read(void)
{
	return asi_u64_read(ASI_DMMU, VA_DMMU_TAG_ACCESS);
}

/** Write IMMU TLB Data in Register.
 *
 * @param v		Value to be written.
 */
_NO_TRACE static inline void itlb_data_in_write(uint64_t v)
{
	asi_u64_write(ASI_ITLB_DATA_IN_REG, 0, v);
	flush_pipeline();
}

/** Write DMMU TLB Data in Register.
 *
 * @param v		Value to be written.
 */
_NO_TRACE static inline void dtlb_data_in_write(uint64_t v)
{
	asi_u64_write(ASI_DTLB_DATA_IN_REG, 0, v);
	membar();
}

/** Read ITLB Synchronous Fault Status Register.
 *
 * @return		Current content of I-SFSR register.
 */
_NO_TRACE static inline uint64_t itlb_sfsr_read(void)
{
	return asi_u64_read(ASI_IMMU, VA_IMMU_SFSR);
}

/** Write ITLB Synchronous Fault Status Register.
 *
 * @param v		New value of I-SFSR register.
 */
_NO_TRACE static inline void itlb_sfsr_write(uint64_t v)
{
	asi_u64_write(ASI_IMMU, VA_IMMU_SFSR, v);
	flush_pipeline();
}

/** Read DTLB Synchronous Fault Status Register.
 *
 * @return		Current content of D-SFSR register.
 */
_NO_TRACE static inline uint64_t dtlb_sfsr_read(void)
{
	return asi_u64_read(ASI_DMMU, VA_DMMU_SFSR);
}

/** Write DTLB Synchronous Fault Status Register.
 *
 * @param v		New value of D-SFSR register.
 */
_NO_TRACE static inline void dtlb_sfsr_write(uint64_t v)
{
	asi_u64_write(ASI_DMMU, VA_DMMU_SFSR, v);
	membar();
}

/** Read DTLB Synchronous Fault Address Register.
 *
 * @return		Current content of D-SFAR register.
 */
_NO_TRACE static inline uint64_t dtlb_sfar_read(void)
{
	return asi_u64_read(ASI_DMMU, VA_DMMU_SFAR);
}

/** Perform IMMU TLB Demap Operation.
 *
 * @param type		Selects between context and page demap (and entire MMU
 * 			demap on US3).
 * @param context_encoding Specifies which Context register has Context ID for
 * 			demap.
 * @param page		Address which is on the page to be demapped.
 */
_NO_TRACE static inline void itlb_demap(int type, int context_encoding, uintptr_t page)
{
	tlb_demap_addr_t da;
	page_address_t pg;

	da.value = 0;
	pg.address = page;

	da.type = type;
	da.context = context_encoding;
	da.vpn = pg.vpn;

	/* da.value is the address within the ASI */
	asi_u64_write(ASI_IMMU_DEMAP, da.value, 0);

	flush_pipeline();
}

/** Perform DMMU TLB Demap Operation.
 *
 * @param type		Selects between context and page demap (and entire MMU
 * 			demap on US3).
 * @param context_encoding Specifies which Context register has Context ID for
 * 			demap.
 * @param page		Address which is on the page to be demapped.
 */
_NO_TRACE static inline void dtlb_demap(int type, int context_encoding, uintptr_t page)
{
	tlb_demap_addr_t da;
	page_address_t pg;

	da.value = 0;
	pg.address = page;

	da.type = type;
	da.context = context_encoding;
	da.vpn = pg.vpn;

	/* da.value is the address within the ASI */
	asi_u64_write(ASI_DMMU_DEMAP, da.value, 0);

	membar();
}

extern void fast_instruction_access_mmu_miss(unsigned int, istate_t *);
extern void fast_data_access_mmu_miss(unsigned int, istate_t *);
extern void fast_data_access_protection(unsigned int, istate_t *);

extern void dtlb_insert_mapping(uintptr_t, uintptr_t, int, bool, bool);

extern void dump_sfsr_and_sfar(void);
extern void describe_dmmu_fault(void);

#endif /* !def __ASSEMBLER__ */

#endif

/** @}
 */
