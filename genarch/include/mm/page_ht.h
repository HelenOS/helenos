/*
 * Copyright (C) 2006 Jakub Jermar
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

/*
 * This is the generic page hash table interface.
 * Architectures that use single page hash table for
 * storing page translations must implement it.
 */

#ifndef __PAGE_HT_H__
#define __PAGE_HT_H__

#include <mm/page.h>
#include <typedefs.h>

/** Number of slots in page hash table. */
#define HT_ENTRIES			HT_ENTRIES_ARCH

/** Hash function.
 *
 * @param page Virtual address. Only vpn bits will be used.
 * @param asid Address space identifier.
 *
 * @return Pointer to hash table typed pte_t *.
 */
#define HT_HASH(page, asid)		HT_HASH_ARCH(page, asid)

/** Compare PTE with page and asid.
 *
 * @param page Virtual address. Only vpn bits will be used.
 * @param asid Address space identifier.
 * @param t PTE.
 *
 * @return 1 on match, 0 otherwise.
 */
#define HT_COMPARE(page, asid, t)	HT_COMPARE_ARCH(page, asid, t)

/** Identify empty page hash table slots.
 *
 * @param t Pointer ro hash table typed pte_t *.
 *
 * @return 1 if the slot is empty, 0 otherwise.
 */
#define HT_SLOT_EMPTY(t)		HT_SLOT_EMPTY_ARCH(t)

/** Invalidate/empty page hash table slot.
 *
 * @param t Address of the slot to be invalidated.
 */
#define HT_INVALIDATE_SLOT(t)		HT_INVALIDATE_SLOT_ARCH(t)

/** Return next record in collision chain.
 *
 * @param t PTE.
 *
 * @return Successor of PTE or NULL.
 */
#define HT_GET_NEXT(t)			HT_GET_NEXT_ARCH(t)

/** Set successor in collision chain.
 *
 * @param t PTE.
 * @param s Successor or NULL.
 */
#define HT_SET_NEXT(t, s)		HT_SET_NEXT_ARCH(t, s)

/** Set page hash table record.
 *
 * @param t PTE.
 * @param page Virtual address. Only vpn bits will be used.
 * @param asid Address space identifier.
 * @param frame Physical address. Only pfn bits will be used.
 * @param flags Flags. See mm/page.h.
 */
#define HT_SET_RECORD(t, page, asid, frame, flags)	HT_SET_RECORD_ARCH(t, page, asid, frame, flags)


extern page_operations_t page_ht_operations;
extern spinlock_t page_ht_lock;

extern pte_t *page_ht;

extern void ht_invalidate_all(void);

#endif
