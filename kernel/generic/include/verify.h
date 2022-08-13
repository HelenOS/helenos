/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_VERIFY_H_
#define KERN_VERIFY_H_

#ifdef CONFIG_VERIFY_VCC

#define READS(ptr)     __specification(reads(ptr))
#define WRITES(ptr)    __specification(writes(ptr))
#define REQUIRES(...)  __specification(requires __VA_ARGS__)

#define EXTENT(ptr)              \extent(ptr)
#define ARRAY_RANGE(ptr, nmemb)  \array_range(ptr, nmemb)

#define REQUIRES_EXTENT_MUTABLE(ptr) \
	REQUIRES(\extent_mutable(ptr))

#define REQUIRES_ARRAY_MUTABLE(ptr, nmemb) \
	REQUIRES(\mutable_array(ptr, nmemb))

#else /* CONFIG_VERIFY_VCC */

#define READS(ptr)
#define WRITES(ptr)
#define REQUIRES(...)

#define EXTENT(ptr)
#define ARRAY_RANGE(ptr, nmemb)

#define REQUIRES_EXTENT_MUTABLE(ptr)
#define REQUIRES_ARRAY_MUTABLE(ptr, nmemb)

#endif /* CONFIG_VERIFY_VCC */

#endif

/** @}
 */
