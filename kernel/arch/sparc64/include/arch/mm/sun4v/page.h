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

#ifndef KERN_sparc64_sun4v_PAGE_H_
#define KERN_sparc64_sun4v_PAGE_H_

#include <arch/mm/frame.h>

#define MMU_PAGE_WIDTH	MMU_FRAME_WIDTH
#define MMU_PAGE_SIZE	MMU_FRAME_SIZE

#define PAGE_WIDTH	FRAME_WIDTH
#define PAGE_SIZE	FRAME_SIZE

#define MMU_PAGES_PER_PAGE	(1 << (PAGE_WIDTH - MMU_PAGE_WIDTH))

#ifndef __ASSEMBLER__

#include <arch/interrupt.h>

extern uintptr_t physmem_base;

#define KA2PA(x)	(((uintptr_t) (x)) + physmem_base)
#define PA2KA(x)	(((uintptr_t) (x)) - physmem_base)

typedef union {
	uintptr_t address;
	struct {
		uint64_t vpn : 51;		/**< Virtual Page Number. */
		unsigned offset : 13;		/**< Offset. */
	} __attribute__((packed));
} page_address_t;

extern void page_arch_init(void);

#endif /* !def __ASSEMBLER__ */

#endif

/** @}
 */
