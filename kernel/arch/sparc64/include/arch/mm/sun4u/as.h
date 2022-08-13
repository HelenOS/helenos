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

#ifndef KERN_sparc64_sun4u_AS_H_
#define KERN_sparc64_sun4u_AS_H_

#include <arch/mm/tte.h>

#define KERNEL_ADDRESS_SPACE_SHADOWED_ARCH  1
#define KERNEL_SEPARATE_PTL0_ARCH           0

#define KERNEL_ADDRESS_SPACE_START_ARCH  UINT64_C(0x0000000000000000)
#define KERNEL_ADDRESS_SPACE_END_ARCH    UINT64_C(0xffffffffffffffff)
#define USER_ADDRESS_SPACE_START_ARCH    UINT64_C(0x0000000000000000)
#define USER_ADDRESS_SPACE_END_ARCH      UINT64_C(0xffffffffffffffff)

#ifdef CONFIG_TSB

/** TSB Tag Target register. */
typedef union tsb_tag_target {
	uint64_t value;
	struct {
		unsigned invalid : 1;	/**< Invalidated by software. */
		unsigned : 2;
		unsigned context : 13;	/**< Software ASID. */
		unsigned : 6;
		uint64_t va_tag : 42;	/**< Virtual address bits <63:22>. */
	} __attribute__((packed));
} tsb_tag_target_t;

/** TSB entry. */
typedef struct tsb_entry {
	tsb_tag_target_t tag;
	tte_data_t data;
} __attribute__((packed)) tsb_entry_t;

typedef struct {
	tsb_entry_t *itsb;
	tsb_entry_t *dtsb;
} as_arch_t;

#else

typedef struct {
} as_arch_t;

#endif /* CONFIG_TSB */

#include <genarch/mm/as_ht.h>

#ifdef CONFIG_TSB
#include <arch/mm/tsb.h>
#define as_invalidate_translation_cache(as, page, cnt) \
	tsb_invalidate((as), (page), (cnt))
#else
#define as_invalidate_translation_cache(as, page, cnt)
#endif

extern void as_arch_init(void);

#endif

/** @}
 */
