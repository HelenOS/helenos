/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_ATOMIC_H_
#define KERN_ATOMIC_H_

#include <stdbool.h>
#include <typedefs.h>
#include <stdatomic.h>

#define atomic_predec(val) \
	(atomic_fetch_sub((val), 1) - 1)

#define atomic_preinc(val) \
	(atomic_fetch_add((val), 1) + 1)

#define atomic_postdec(val) \
	atomic_fetch_sub((val), 1)

#define atomic_postinc(val) \
	atomic_fetch_add((val), 1)

#define atomic_dec(val) \
	((void) atomic_fetch_sub(val, 1))

#define atomic_inc(val) \
	((void) atomic_fetch_add(val, 1))

#define local_atomic_exchange(var_addr, new_val) \
	atomic_exchange_explicit( \
	    (_Atomic typeof(*(var_addr)) *) (var_addr), \
	    (new_val), memory_order_relaxed)

#endif

/** @}
 */
