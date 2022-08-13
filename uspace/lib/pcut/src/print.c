/*
 * SPDX-FileCopyrightText: 2012-2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
