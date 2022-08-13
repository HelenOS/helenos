/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_mm
 * @{
 */
/** @file
 */

#ifndef KERN_TLB_H_
#define KERN_TLB_H_

#include <arch/mm/asid.h>
#include <typedefs.h>

/**
 * Number of TLB shootdown messages that can be queued in processor tlb_messages
 * queue.
 */
#define TLB_MESSAGE_QUEUE_LEN	10

/** Type of TLB shootdown message. */
typedef enum {
	/** Invalid type. */
	TLB_INVL_INVALID = 0,
	/** Invalidate all entries in TLB. */
	TLB_INVL_ALL,
	/** Invalidate all entries belonging to one address space. */
	TLB_INVL_ASID,
	/** Invalidate specified page range belonging to one address space. */
	TLB_INVL_PAGES
} tlb_invalidate_type_t;

/** TLB shootdown message. */
typedef struct {
	tlb_invalidate_type_t type;	/**< Message type. */
	asid_t asid;			/**< Address space identifier. */
	uintptr_t page;			/**< Page address. */
	size_t count;			/**< Number of pages to invalidate. */
} tlb_shootdown_msg_t;

extern void tlb_init(void);

#ifdef CONFIG_SMP
extern ipl_t tlb_shootdown_start(tlb_invalidate_type_t, asid_t, uintptr_t,
    size_t);
extern void tlb_shootdown_finalize(ipl_t);
extern void tlb_shootdown_ipi_recv(void);
#else
#define tlb_shootdown_start(w, x, y, z)	interrupts_disable()
#define tlb_shootdown_finalize(i)	(interrupts_restore(i));
#define tlb_shootdown_ipi_recv()
#endif /* CONFIG_SMP */

/* Export TLB interface that each architecture must implement. */
extern void tlb_arch_init(void);
extern void tlb_print(void);
extern void tlb_shootdown_ipi_send(void);

extern void tlb_invalidate_all(void);
extern void tlb_invalidate_asid(asid_t);
extern void tlb_invalidate_pages(asid_t, uintptr_t, size_t);

#endif

/** @}
 */
