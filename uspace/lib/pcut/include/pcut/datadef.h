/*
 * SPDX-FileCopyrightText: 2012-2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 * Data types internally used by PCUT.
 */
#ifndef PCUT_DATADEF_H_GUARD
#define PCUT_DATADEF_H_GUARD

#include <pcut/prevs.h>
#include <stdlib.h>
#include <stddef.h>

/** @cond devel */

#if defined(__GNUC__) || defined(__clang__)
#define PCUT_CC_UNUSED_VARIABLE(name, initializer) \
	name __attribute__((unused)) = initializer
#else
#define PCUT_CC_UNUSED_VARIABLE(name, initializer) \
	name = initializer
#endif


enum {
	PCUT_KIND_SKIP,
	PCUT_KIND_NESTED,
	PCUT_KIND_SETUP,
	PCUT_KIND_TEARDOWN,
	PCUT_KIND_TESTSUITE,
	PCUT_KIND_TEST
};

enum {
	PCUT_EXTRA_TIMEOUT,
	PCUT_EXTRA_SKIP,
	PCUT_EXTRA_LAST
};

enum {
	PCUT_MAIN_EXTRA_PREINIT_HOOK,
	PCUT_MAIN_EXTRA_INIT_HOOK,
	PCUT_MAIN_EXTRA_REPORT_XML,
	PCUT_MAIN_EXTRA_LAST
};

/** Generic wrapper for test cases, test suites etc. */
typedef struct pcut_item pcut_item_t;

/** Extra information about a test. */
typedef struct pcut_extra pcut_extra_t;

/** Extra information for the main() function. */
typedef struct pcut_main_extra pcut_main_extra_t;

/** Test method type. */
typedef void (*pcut_test_func_t)(void);

/** Set-up or tear-down method type. */
typedef void (*pcut_setup_func_t)(void);

/** @copydoc pcut_extra_t */
struct pcut_extra {
	/** Discriminator for the union.
	 *
	 * Use PCUT_EXTRA_* to determine which field of the union is used.
	 */
	int type;
	/** Test-specific time-out in seconds. */
	int timeout;
};

/** @copydoc pcut_main_extra_t */
struct pcut_main_extra {
	/** Discriminator for the union.
	 *
	 * Use PCUT_MAIN_EXTRA_* to determine which field of the union is used.
	 */
	int type;
	/** Callback once PCUT initializes itself. */
	void (*init_hook)(void);
	/** Callback even before command-line arguments are processed. */
	void (*preinit_hook)(int *, char ***);
};

/** @copydoc pcut_item_t */
struct pcut_item {
	/** Link to previous item. */
	pcut_item_t *previous;
	/** Link to next item. */
	pcut_item_t *next;

	/** Unique id of this item. */
	int id;

	/** Discriminator for this item. */
	int kind;

	/** Name of this item. */
	const char *name;

	/** Test-case function. */
	pcut_test_func_t test_func;

	/** Set-up function of a suite. */
	pcut_setup_func_t setup_func;
	/** Tear-down function of a suite. */
	pcut_setup_func_t teardown_func;

	/** Extra attributes. */
	pcut_extra_t *extras;

	/** Extra attributes for main() function. */
	pcut_main_extra_t *main_extras;

	/** Nested lists. */
	pcut_item_t *nested;
};

/** @endcond */

#endif
