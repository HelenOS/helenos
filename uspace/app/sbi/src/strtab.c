/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file String table.
 *
 * Converts strings to more compact SID (string ID, integer) and back.
 * (The point is that this deduplicates the strings. Using SID might actually
 * not be such a big win.)
 *
 * The string table is a singleton as there will never be a need for
 * more than one.
 *
 * Current implementation uses a linked list and thus it is slow.
 */

#include <stdlib.h>
#include "mytypes.h"
#include "os/os.h"
#include "list.h"

#include "strtab.h"

static list_t str_list;

/** Initialize string table. */
void strtab_init(void)
{
	list_init(&str_list);
}

/** Get SID of a string.
 *
 * Return SID of @a str. If @a str is not in the string table yet,
 * it is added and thus a new SID is assigned.
 *
 * @param str	String
 * @return	SID of @a str.
 */
sid_t strtab_get_sid(const char *str)
{
	list_node_t *node;
	sid_t sid;

	sid = 0;
	node = list_first(&str_list);

	while (node != NULL) {
		++sid;
		if (os_str_cmp(str, list_node_data(node, char *)) == 0)
			return sid;

		node = list_next(&str_list, node);
	}

	++sid;
	list_append(&str_list, os_str_dup(str));

	return sid;
}

/** Get string with the given SID.
 *
 * Returns string that has SID @a sid. If no such string exists, this
 * causes a fatal error in the interpreter.
 *
 * @param sid	SID of the string.
 * @return	Pointer to the string.
 */
char *strtab_get_str(sid_t sid)
{
	list_node_t *node;
	sid_t cnt;

	node = list_first(&str_list);
	cnt = 1;
	while (node != NULL && cnt < sid) {
		node = list_next(&str_list, node);
		cnt += 1;
	}

	if (node == NULL) {
		printf("Internal error: Invalid SID %d", sid);
		abort();
	}

	return list_node_data(node, char *);
}
