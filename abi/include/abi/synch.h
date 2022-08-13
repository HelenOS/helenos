/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup abi_generic
 * @{
 */
/** @file
 */

#ifndef _ABI_SYNCH_H_
#define _ABI_SYNCH_H_

enum {
	/** Request with no timeout. */
	SYNCH_NO_TIMEOUT = 0,
};

enum {
	/** No flags specified. */
	SYNCH_FLAGS_NONE          = 0,
	/** Non-blocking operation request. */
	SYNCH_FLAGS_NON_BLOCKING  = 1 << 0,
	/** Interruptible operation. */
	SYNCH_FLAGS_INTERRUPTIBLE = 1 << 1,
	/** Futex operation (makes sleep with timeout composable). */
	SYNCH_FLAGS_FUTEX         = 1 << 2,
};

#endif

/** @}
 */
