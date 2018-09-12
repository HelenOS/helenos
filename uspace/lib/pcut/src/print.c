/*
 * Copyright (c) 2012-2013 Vojtech Horky
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

/** @file
 *
 * Helper functions for debugging prints.
 */

#include <pcut/pcut.h>

#pragma warning(push, 0)
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#pragma warning(pop)

#include "internal.h"

/** Print all items in the given list.
 *
 * @param first First item to be printed.
 */
void pcut_print_items(pcut_item_t *first) {
	pcut_item_t *it = first;
	printf("====>\n");
	while (it != NULL) {
		switch (it->kind) {
		case PCUT_KIND_TEST:
			printf("TEST %s\n", it->name);
			break;
		case PCUT_KIND_TESTSUITE:
			printf("SUITE %s\n", it->name);
			break;
		case PCUT_KIND_SKIP:
			break;
		case PCUT_KIND_NESTED:
			printf("NESTED ...\n");
			break;
		default:
			printf("UNKNOWN (%d)\n", it->kind);
			break;
		}
		it = it->next;
	}
	printf("----\n");
}

/** Print valid items in the list.
 *
 * @param first First item to be printed.
 */
void pcut_print_tests(pcut_item_t *first) {
	pcut_item_t *it;
	for (it = pcut_get_real(first); it != NULL; it = pcut_get_real_next(it)) {
		switch (it->kind) {
		case PCUT_KIND_TESTSUITE:
			printf("  Suite `%s' [%d]\n", it->name, it->id);
			break;
		case PCUT_KIND_TEST:
			printf("    Test `%s' [%d]\n", it->name, it->id);
			break;
		case PCUT_KIND_SETUP:
		case PCUT_KIND_TEARDOWN:
			/* Fall-through, do nothing. */
			break;
		default:
			assert(0 && "unreachable case in item-kind switch");
			break;
		}
	}
}
