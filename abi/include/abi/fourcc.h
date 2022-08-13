/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _ABI_FOURCC_H_
#define _ABI_FOURCC_H_

#include <stdint.h>

#define FOURCC(a, b, c, d) \
	(((uint32_t) (a)) | (((uint32_t) (b)) << 8) | \
	    (((uint32_t) (c)) << 16) | (((uint32_t) (d)) << 24))

#define CC_COMPACT(a) \
	((uint32_t) (a) & 0x7f)

#define FOURCC_COMPACT(a, b, c, d) \
	((CC_COMPACT(a) << 4) | (CC_COMPACT(b) << 11) | \
	    (CC_COMPACT(c) << 18) | (CC_COMPACT(d) << 25))

#endif

/** @}
 */
