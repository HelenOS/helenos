/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_sun4u_TTE_H_
#define KERN_sparc64_sun4u_TTE_H_

#define TTE_G		(1 << 0)
#define TTE_W		(1 << 1)
#define TTE_P		(1 << 2)
#define TTE_E		(1 << 3)
#define TTE_CV		(1 << 4)
#define TTE_CP		(1 << 5)
#define TTE_L		(1 << 6)

#define TTE_V_SHIFT	63
#define TTE_SIZE_SHIFT	61

#ifndef __ASSEMBLER__

#include <stdint.h>

/* TTE tag's VA_tag field contains bits <63:VA_TAG_PAGE_SHIFT> of the VA */
#define VA_TAG_PAGE_SHIFT	22

/** Translation Table Entry - Tag. */
union tte_tag {
	uint64_t value;
	struct {
		unsigned g : 1;		/**< Global. */
		unsigned : 2;		/**< Reserved. */
		unsigned context : 13;	/**< Context identifier. */
		unsigned : 6;		/**< Reserved. */
		uint64_t va_tag : 42;	/**< Virtual Address Tag, bits 63:22. */
	} __attribute__((packed));
};

typedef union tte_tag tte_tag_t;

/** Translation Table Entry - Data. */
union tte_data {
	uint64_t value;
	struct {
		unsigned v : 1;		/**< Valid. */
		unsigned size : 2;	/**< Page size of this entry. */
		unsigned nfo : 1;	/**< No-Fault-Only. */
		unsigned ie : 1;	/**< Invert Endianness. */
		unsigned soft2 : 9;	/**< Software defined field. */
#if defined (US)
		unsigned diag : 9;	/**< Diagnostic data. */
		unsigned pfn : 28;	/**< Physical Address bits, bits 40:13. */
#elif defined (US3)
		unsigned : 7;		/**< Reserved. */
		unsigned pfn : 30;	/**< Physical Address bits, bits 42:13 */
#endif
		unsigned soft : 6;	/**< Software defined field. */
		unsigned l : 1;		/**< Lock. */
		unsigned cp : 1;	/**< Cacheable in physically indexed cache. */
		unsigned cv : 1;	/**< Cacheable in virtually indexed cache. */
		unsigned e : 1;		/**< Side-effect. */
		unsigned p : 1;		/**< Privileged. */
		unsigned w : 1;		/**< Writable. */
		unsigned g : 1;		/**< Global. */
	} __attribute__((packed));
};

typedef union tte_data tte_data_t;

#endif /* !def __ASSEMBLER__ */

#endif

/** @}
 */
