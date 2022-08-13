/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdrv
 * @{
 */

#include <stdio.h>
#include <stddef.h>
#include <ddf/log.h>
#include <assert.h>
#include <str.h>

/** Formatting string for printing number of not-printed items. */
#define REMAINDER_STR_FMT " (%zu)..."
/** Expected max size of the remainder string.
 * String + terminator + number width (enough for 4GB).
 */
#define REMAINDER_STR_LEN (5 + 1 + 10)

/** Groups size. */
#define BUFFER_DUMP_GROUP_SIZE 4

/** Space between two items. */
#define SPACE_NORMAL " "
/** Space between two groups. */
#define SPACE_GROUP "  "

/** Formats the dump with space before, takes care of type casting (ugly). */
#define _FORMAT(digits, bits) \
	snprintf(dump, dump_size, "%s%0" #digits PRIx##bits, \
	    space_before, ((uint##bits##_t *)buf)[0]);

/** Dump one item into given buffer.
 *
 * @param buffer Data buffer.
 * @param item_size Size of the item (1, 2, 4).
 * @param index Index into the @p buffer (respecting @p item_size).
 * @param dump Where to store the dump.
 * @param dump_size Size of @p dump size.
 * @return Number of characters printed (see snprintf).
 */
static int dump_one_item(const void *buffer, size_t item_size, size_t index,
    char *dump, size_t dump_size)
{
	/* Determine space before the number. */
	const char *space_before;
	if (index == 0) {
		space_before = "";
	} else if ((index % BUFFER_DUMP_GROUP_SIZE) == 0) {
		space_before = SPACE_GROUP;
	} else {
		space_before = SPACE_NORMAL;
	}

	/* Let buf point to the item to be printed. */
	const uint8_t *buf = (const uint8_t *) buffer;
	buf += index * item_size;

	switch (item_size) {
	case 4:
		return _FORMAT(8, 32);
	case 2:
		return _FORMAT(4, 16);
	default:
		return _FORMAT(2, 8);
	}
}

/** Count number of characters needed for dumping buffer of given size.
 *
 * @param item_size Item size in bytes.
 * @param items Number of items to print.
 * @return Number of characters the full dump would occupy.
 */
static size_t count_dump_length(size_t item_size, size_t items)
{
	size_t group_space_count = items / BUFFER_DUMP_GROUP_SIZE - 1;
	size_t normal_space_count = items - 1 - group_space_count;

	size_t dump_itself = item_size * 2 * items;
	size_t group_spaces = str_size(SPACE_GROUP) * group_space_count;
	size_t normal_spaces = str_size(SPACE_NORMAL) * normal_space_count;

	return dump_itself + group_spaces + normal_spaces;
}

/** Dumps data buffer to a string in hexadecimal format.
 *
 * Setting @p items_to_print to zero would dump the whole buffer together
 * with information how many items were omitted. Otherwise, no information
 * about omitted items is printed.
 *
 * @param dump Where to store the dumped buffer.
 * @param dump_size Size of @p dump in bytes.
 * @param buffer Data buffer to be dumped.
 * @param item_size Size of items in the @p buffer in bytes (1,2,4 allowed).
 * @param items Number of items in the @p buffer.
 * @param items_to_print How many items to actually print.
 */
void ddf_dump_buffer(char *dump, size_t dump_size,
    const void *buffer, size_t item_size, size_t items, size_t items_to_print)
{
	if ((dump_size == 0) || (dump == NULL)) {
		return;
	}
	/* We need space for one byte at least. */
	if (dump_size < 3) {
		str_cpy(dump, dump_size, "...");
		return;
	}

	/* Special cases first. */
	if (buffer == NULL) {
		str_cpy(dump, dump_size, "(null)");
		return;
	}
	if (items == 0) {
		str_cpy(dump, dump_size, "(empty)");
	}

	if (items_to_print > items) {
		items_to_print = items;
	}

	bool print_remainder = items_to_print == 0;

	/* How many available bytes we do have. */
	size_t dump_size_remaining = dump_size - 1;

	if (print_remainder) {
		/* Can't do much when user supplied small buffer. */
		if (dump_size_remaining < REMAINDER_STR_LEN) {
			print_remainder = false;
		} else {
			size_t needed_size = count_dump_length(item_size, items);
			if (needed_size > dump_size_remaining) {
				dump_size_remaining -= REMAINDER_STR_LEN;
			} else {
				print_remainder = false;
			}
		}
		items_to_print = items;
	}

	str_cpy(dump, dump_size, "");

	size_t index = 0;
	while (index < items) {
		char current_item[32];
		int printed = dump_one_item(buffer, item_size, index,
		    current_item, 32);
		assert(printed >= 0);

		if ((size_t) printed > dump_size_remaining) {
			break;
		}

		str_append(dump, dump_size, current_item);

		dump_size_remaining -= printed;
		index++;

		if (index >= items_to_print) {
			break;
		}
	}

	if (print_remainder && (index < items)) {
		size_t s = str_size(dump);
		snprintf(dump + s, dump_size - s, REMAINDER_STR_FMT,
		    items - index);
	}
}

/** @}
 */
