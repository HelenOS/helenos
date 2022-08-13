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

#ifndef KERN_COPY_H_
#define KERN_COPY_H_

#include <stddef.h>

/** Label within memcpy_from_uspace() that contains return -1. */
extern char memcpy_from_uspace_failover_address;

/** Label within memcpy_to_uspace() that contains return -1. */
extern char memcpy_to_uspace_failover_address;

extern errno_t copy_from_uspace(void *dst, uspace_addr_t uspace_src, size_t size);
extern errno_t copy_to_uspace(uspace_addr_t dst_uspace, const void *src, size_t size);

/*
 * This interface must be implemented by each architecture.
 * The functions return zero on failure and nonzero on success.
 */
extern uintptr_t memcpy_from_uspace(void *dst, uspace_addr_t uspace_src, size_t size);
extern uintptr_t memcpy_to_uspace(uspace_addr_t uspace_dst, const void *src, size_t size);

#endif

/** @}
 */
