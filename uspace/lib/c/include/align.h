/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_ALIGN_H_
#define _LIBC_ALIGN_H_

/** Align to the nearest lower address which is a power of two.
 *
 * @param s		Address or size to be aligned.
 * @param a		Size of alignment, must be power of 2.
 */
#define ALIGN_DOWN(s, a)	((s) & ~((a) - 1))

/** Align to the nearest higher address which is a power of two.
 *
 * @param s		Address or size to be aligned.
 * @param a		Size of alignment, must be power of 2.
 */
#define ALIGN_UP(s, a)		((long)((s) + ((a) - 1)) & ~((long) (a) - 1))

/** Round up to the nearest higher boundary.
 *
 * @param n		Number to be aligned.
 * @param b		Boundary, arbitrary unsigned number.
 */
#define ROUND_UP(n, b)		(((n) / (b) + ((n) % (b) != 0)) * (b))

#endif

/** @}
 */
