/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 * SPDX-FileCopyrightText: 2013 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup abi
 * @{
 */
/** @file
 */

#ifndef _ABI_LOG_H_
#define _ABI_LOG_H_

/** Log message level. */
typedef enum {
	/** Fatal error, program is not able to recover at all. */
	LVL_FATAL,
	/** Serious error but the program can recover from it. */
	LVL_ERROR,
	/** Easily recoverable problem. */
	LVL_WARN,
	/** Information message that ought to be printed by default. */
	LVL_NOTE,
	/** Debugging purpose message. */
	LVL_DEBUG,
	/** More detailed debugging message. */
	LVL_DEBUG2,

	/** For checking range of values */
	LVL_LIMIT
} log_level_t;

/* Who is the source of the message? */
typedef enum {
	LF_OTHER = 0,
	LF_USPACE,
	LF_ARCH
} log_facility_t;

#endif

/** @}
 */
