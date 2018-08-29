/*
 * Copyright (c) 2009 Jiri Svoboda
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
