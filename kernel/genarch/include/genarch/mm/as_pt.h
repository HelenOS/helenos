/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch_mm
 * @{
 */
/** @file
 */

#ifndef KERN_AS_PT_H_
#define KERN_AS_PT_H_

#include <arch/mm/page.h>

#define AS_PAGE_TABLE

typedef struct {
	/** Page table pointer. */
	pte_t *page_table;
} as_genarch_t;

#endif

/** @}
 */
