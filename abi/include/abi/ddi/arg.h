/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup abi_generic
 * @{
 */
/** @file
 */

#ifndef _ABI_DDI_ARG_H_
#define _ABI_DDI_ARG_H_

#include <stdint.h>

enum {
	DMAMEM_FLAGS_ANONYMOUS = 0x01,
};

/** Structure encapsulating arguments for SYS_PHYSMEM_MAP syscall. */
typedef struct {
	/** ID of the destination task. */
	uint64_t task_id;
	/** Physical address of starting frame. */
	void *phys_base;
	/** Virtual address of starting page. */
	void *virt_base;
	/** Number of pages to map. */
	size_t pages;
	/** Address space area flags for the mapping. */
	unsigned int flags;
} ddi_memarg_t;

/** Structure encapsulating arguments for SYS_ENABLE_IOSPACE syscall. */
typedef struct {
	uint64_t task_id;  /**< ID of the destination task. */
	void *ioaddr;      /**< Starting I/O space address. */
	size_t size;       /**< Number of bytes. */
} ddi_ioarg_t;

#endif

/** @}
 */
