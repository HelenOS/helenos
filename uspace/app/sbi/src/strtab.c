/*
 * Copyright (c) 2010 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @file String table.
 *
 * Converts strings to more compact SID (string ID, integer) and back.
 * The string table is not an object as there will never be a need for
 * more than one.
 */

#include <stdlib.h>
#include <string.h>
#include "mytypes.h"
#include "compat.h"
#include "list.h"

#include "strtab.h"

static list_t str_list;

void strtab_init(void)
{
	list_init(&str_list);
}

sid_t strtab_get_sid(char *str)
{
	list_node_t *node;
	sid_t sid;

	sid = 0;
	node = list_first(&str_list);

	while (node != NULL) {
		++sid;
		if (strcmp(str, list_node_data(node, char *)) == 0)
			return sid;

		node = list_next(&str_list, node);
	}

	++sid;
	list_append(&str_list, strdup(str));

	return sid;
}

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
		exit(1);
	}

	return list_node_data(node, char *);
}
