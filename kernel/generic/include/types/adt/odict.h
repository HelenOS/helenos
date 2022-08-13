/*
 * SPDX-FileCopyrightText: 2016 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_adt
 * @{
 */
/** @file
 */

#ifndef _LIBC_TYPES_ODICT_H_
#define _LIBC_TYPES_ODICT_H_

#include <adt/list.h>

typedef struct odlink odlink_t;
typedef struct odict odict_t;

typedef void *(*odgetkey_t)(odlink_t *);
typedef int (*odcmp_t)(void *, void *);

typedef enum {
	odc_black,
	odc_red
} odict_color_t;

typedef enum {
	/** Child A */
	odcs_a,
	/** Child B */
	odcs_b
} odict_child_sel_t;

/** Ordered dictionary link */
struct odlink {
	/** Containing dictionary */
	odict_t *odict;
	/** Parent node */
	odlink_t *up;
	/** First child */
	odlink_t *a;
	/** Second child */
	odlink_t *b;
	/** Node color */
	odict_color_t color;
	/** Link to odict->entries */
	link_t lentries;
};

/** Ordered dictionary */
struct odict {
	/** Root of the tree */
	odlink_t *root;
	/** List of entries in ascending order */
	list_t entries;
	/** Get key operation */
	odgetkey_t getkey;
	/** Compare operation */
	odcmp_t cmp;
};

#endif

/** @}
 */
