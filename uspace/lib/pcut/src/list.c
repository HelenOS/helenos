/*
 * SPDX-FileCopyrightText: 2012-2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 *
 * Helper functions for working with list of items.
 */

#pragma warning(push, 0)
#include <assert.h>
#include <stdlib.h>
#pragma warning(pop)

#include "internal.h"
#include <pcut/pcut.h>


/** Find next item with actual content.
 *
 * @param item Head of the list.
 * @return First item with actual content or NULL on end of list.
 */
pcut_item_t *pcut_get_real_next(pcut_item_t *item) {
	if (item == NULL) {
		return NULL;
	}

	do {
		item = item->next;
	} while ((item != NULL) && (item->kind == PCUT_KIND_SKIP));


	return item;
}

/** Retrieve the first item with actual content.
 *
 * Unlike pcut_get_real_next(), where we always advance after the
 * first item, here the @p item itself could be returned.
 *
 * @param item Head of the list.
 * @return First item with actual content or NULL on end of list.
 */
pcut_item_t *pcut_get_real(pcut_item_t *item) {
	if (item == NULL) {
		return NULL;
	}

	if (item->kind == PCUT_KIND_SKIP) {
		return pcut_get_real_next(item);
	} else {
		return item;
	}
}


/** In-line nested lists into the parent.
 *
 * @param nested Head of the nested list.
 */
static void inline_nested_lists(pcut_item_t *nested) {
	pcut_item_t *first;

	if (nested->kind != PCUT_KIND_NESTED) {
		return;
	}

	if (nested->nested == NULL) {
		nested->kind = PCUT_KIND_SKIP;
		return;
	}

	first = pcut_fix_list_get_real_head(nested->nested);
	nested->nested->next = nested->next;
	if (nested->next != NULL) {
		nested->next->previous = nested->nested;
	}
	nested->next = first;
	first->previous = nested;

	nested->kind = PCUT_KIND_SKIP;
}

/** Assing unique ids to each item in the list.
 *
 * @param first List head.
 */
static void set_ids(pcut_item_t *first) {
	int id = 1;
	pcut_item_t *it;

	assert(first != NULL);

	if (first->kind == PCUT_KIND_SKIP) {
		first = pcut_get_real_next(first);
	}

	for (it = first; it != NULL; it = pcut_get_real_next(it)) {
		it->id = id;
		id++;
	}
}

/** Hide tests that are marked to be skipped.
 *
 * Go through all tests and those that have PCUT_EXTRA_SKIP mark
 * as skipped with PCUT_KIND_SKIP.
 *
 * @param first Head of the list.
 */
static void detect_skipped_tests(pcut_item_t *first) {
	pcut_item_t *it;

	assert(first != NULL);
	if (first->kind == PCUT_KIND_SKIP) {
		first = pcut_get_real_next(first);
	}

	for (it = first; it != NULL; it = pcut_get_real_next(it)) {
		pcut_extra_t *extras;

		if (it->kind != PCUT_KIND_TEST) {
			continue;
		}

		extras = it->extras;
		while (extras->type != PCUT_EXTRA_LAST) {
			if (extras->type == PCUT_EXTRA_SKIP) {
				it->kind = PCUT_KIND_SKIP;
				break;
			}
			extras++;
		}
	}
}

/** Convert the static single-linked list into a flat double-linked list.
 *
 * The conversion includes
 * * adding forward links
 * * flattening of any nested lists
 * * assigning of unique ids
 *
 * @param last Tail of the list.
 * @return Head of the fixed list.
 */
pcut_item_t *pcut_fix_list_get_real_head(pcut_item_t *last) {
	pcut_item_t *next, *it;

	last->next = NULL;

	inline_nested_lists(last);

	next = last;
	it = last->previous;
	while (it != NULL) {
		it->next = next;
		inline_nested_lists(it);
		next = it;
		it = it->previous;
	}

	detect_skipped_tests(next);

	set_ids(next);

	return next;
}

/** Compute the number of all tests in a list.
 *
 * @param it Head of the list.
 * @return Number of tests.
 */
int pcut_count_tests(pcut_item_t *it) {
	int count = 0;
	while (it != NULL) {
		if (it->kind == PCUT_KIND_TEST) {
			count++;
		}
		it = pcut_get_real_next(it);
	}
	return count;
}
