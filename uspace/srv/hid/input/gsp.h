/*
 * SPDX-FileCopyrightText: 2009 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup inputgen generic
 * @ingroup  input
 * @{
 */

/** @file
 * @brief Generic scancode parser
 */

#ifndef KBD_GSP_H_
#define KBD_GSP_H_

#include <adt/hash_table.h>

enum {
	GSP_END		= -1,	/**< Terminates a sequence. */
	GSP_DEFAULT	= -2	/**< Wildcard, catches unhandled cases. */
};

/** Scancode parser description */
typedef struct {
	/** Transition table, (state, input) -> (state, output) */
	hash_table_t trans;

	/** Number of states */
	int states;
} gsp_t;

/** Scancode parser transition. */
typedef struct {
	ht_link_t link;		/**< Link to hash table in @c gsp_t */

	/* Preconditions */

	int old_state;		/**< State before transition */
	int input;		/**< Input symbol (scancode) */

	/* Effects */

	int new_state;		/**< State after transition */

	/* Output emitted during transition */

	unsigned out_mods;	/**< Modifier to emit */
	unsigned out_key;	/**< Keycode to emit */
} gsp_trans_t;

extern void gsp_init(gsp_t *);
extern int gsp_insert_defs(gsp_t *, const int *);
extern int gsp_insert_seq(gsp_t *, const int *, unsigned, unsigned);
extern int gsp_step(gsp_t *, int, int, unsigned *, unsigned *);

#endif

/**
 * @}
 */
