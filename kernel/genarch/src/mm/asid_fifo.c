/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch_mm
 * @{
 */

/**
 * @file
 * @brief	FIFO queue ASID management.
 *
 * Architectures that link with this file keep the unallocated ASIDs
 * in FIFO queue. The queue can be statically (e.g. mips32) or
 * dynamically allocated (e.g ia64 and sparc64).
 */

#include <genarch/mm/asid_fifo.h>
#include <arch/mm/asid.h>
#include <mm/asid.h>
#include <adt/fifo.h>

#define FIFO_STATIC_LIMIT	1024
#define FIFO_STATIC		(ASIDS_ALLOCABLE<FIFO_STATIC_LIMIT)

/**
 * FIFO queue containing unassigned ASIDs.
 * Can be only accessed when asidlock is held.
 */
#if FIFO_STATIC
FIFO_INITIALIZE_STATIC(free_asids, asid_t, ASIDS_ALLOCABLE);
#else
FIFO_INITIALIZE_DYNAMIC(free_asids, asid_t, ASIDS_ALLOCABLE);
#endif

/** Initialize data structures for O(1) ASID allocation and deallocation. */
void asid_fifo_init(void)
{
	int i;

#if (!FIFO_STATIC)
	fifo_create(free_asids);
	if (!free_asids.fifo)
		panic("Not enough memory to allocate ASID FIFO");
	// TODO: There really is no reason not to statically allocate it
	//       except to keep binary size low. Once kernel is a regular ELF
	//       binary supporting .bss section (wip as of the late 2018),
	//       the dynamic option should be removed.
#endif

	for (i = 0; i < ASIDS_ALLOCABLE; i++) {
		fifo_push(free_asids, ASID_START + i);
	}
}

/** Allocate free ASID.
 *
 * Allocation runs in O(1).
 *
 * @return Free ASID.
 */
asid_t asid_find_free(void)
{
	return fifo_pop(free_asids);
}

/** Return ASID among free ASIDs.
 *
 * This operation runs in O(1).
 *
 * @param asid ASID being freed.
 */
void asid_put_arch(asid_t asid)
{
	fifo_push(free_asids, asid);
}

/** @}
 */
