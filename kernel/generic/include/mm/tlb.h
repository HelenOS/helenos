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

/** @addtogroup genericmm
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
