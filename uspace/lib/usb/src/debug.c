/*
 * SPDX-FileCopyrightText: 2010-2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusb
 * @{
 */
/** @file
 * Debugging and logging support.
 */
#include <fibril_synch.h>
#include <ddf/log.h>
#include <usb/debug.h>
#include <str.h>

#define REMAINDER_STR_FMT " (%zu)..."
/* string + terminator + number width (enough for 4GB) */
#define REMAINDER_STR_LEN (5 + 1 + 10)

/** How many bytes to group together. */
#define BUFFER_DUMP_GROUP_SIZE 4

/** Size of the string for buffer dumps. */
#define BUFFER_DUMP_LEN 240 /* Ought to be enough for everybody ;-). */

/** Fibril local storage for the dumped buffer. */
static fibril_local char buffer_dump[2][BUFFER_DUMP_LEN];
/** Fibril local storage for buffer switching. */
static fibril_local int buffer_dump_index = 0;

/** Dump buffer into string.
 *
 * The function dumps given buffer into hexadecimal format and stores it
 * in a static fibril local string.
 * That means that you do not have to deallocate the string (actually, you
 * can not do that) and you do not have to guard it against concurrent
 * calls to it.
 * The only limitation is that each second call rewrites the buffer again
 * (internally, two buffer are used in cyclic manner).
 * Thus, it is necessary to copy the buffer elsewhere (that includes printing
 * to screen or writing to file).
 * Since this function is expected to be used for debugging prints only,
 * that is not a big limitation.
 *
 * @warning You cannot use this function more than twice in the same printf
 * (see detailed explanation).
 *
 * @param buffer Buffer to be printed (can be NULL).
 * @param size Size of the buffer in bytes (can be zero).
 * @param dumped_size How many bytes to actually dump (zero means all).
 * @return Dumped buffer as a static (but fibril local) string.
 */
const char *usb_debug_str_buffer(const uint8_t *buffer, size_t size,
    size_t dumped_size)
{
	/*
	 * Remove previous string.
	 */
	memset(buffer_dump[buffer_dump_index], 0, BUFFER_DUMP_LEN);

	/* Do the actual dump. */
	ddf_dump_buffer(buffer_dump[buffer_dump_index], BUFFER_DUMP_LEN,
	    buffer, 1, size, dumped_size);

	/* Next time, use the other buffer. */
	buffer_dump_index = 1 - buffer_dump_index;

	/* Need to take the old one due to previous line. */
	return buffer_dump[1 - buffer_dump_index];
}

/**
 * @}
 */
