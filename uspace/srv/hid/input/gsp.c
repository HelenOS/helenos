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

/**
 * @addtogroup kbdgen generic
 * @ingroup  input
 * @{
 */
/** @file
 * @brief Generic scancode parser.
 *
 * The scancode parser is a simple finite state machine. It is described
 * using sequences of input symbols (scancodes) and the corresponding output
 * value (mods, key pair). When the parser recognizes a sequence,
 * it outputs the value and restarts. If a transition is undefined,
 * the parser restarts, too.
 *
 * Apart from precise values, GSP_DEFAULT allows to catch general cases.
 * I.e. if we knew that after 0x1b 0x4f there always follow two more
 * scancodes, we can define (0x1b, 0x4f, GSP_DEFAULT, GSP_DEFAULT, GSP_END)
 * with null output. This will force the parser to read the entire sequence,
 * not leaving garbage on the input if it does not recognize the specific
 * sequence.
 */

#include <adt/hash_table.h>
#include <adt/hash.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "gsp.h"

/*
 * Transition function hash table operations.
 */
typedef struct {
	int old_state;
	int input;
} trans_key_t;

static size_t trans_key_hash(void *key)
{
	trans_key_t *trans_key = (trans_key_t *)key;
	return hash_combine(trans_key->input, trans_key->old_state);
}

static size_t trans_hash(const ht_link_t *item)
{
	gsp_trans_t *t = hash_table_get_inst(item, gsp_trans_t, link);
	return hash_combine(t->input, t->old_state);
}

static bool trans_key_equal(void *key, const ht_link_t *item)
{
	trans_key_t *trans_key = (trans_key_t *)key;
	gsp_trans_t *t = hash_table_get_inst(item, gsp_trans_t, link);

	return trans_key->input == t->input && trans_key->old_state == t->old_state;
}

static hash_table_ops_t trans_ops = {
	.hash = trans_hash,
	.key_hash = trans_key_hash,
	.key_equal = trans_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};


static gsp_trans_t *trans_lookup(gsp_t *p, int state, int input);
static void trans_insert(gsp_t *p, gsp_trans_t *t);
static gsp_trans_t *trans_new(void);

/** Initialize scancode parser. */
void gsp_init(gsp_t *p)
{
	p->states = 1;
	hash_table_create(&p->trans, 0, 0, &trans_ops);
}

/** Insert a series of definitions into the parser.
 *
 * @param p	The parser.
 * @param defs	Definition list. Each definition starts with two output values
 *		(mods, key) and continues with a sequence of input values
 *		terminated with GSP_END. The definition list is terminated
 *		with two zeroes (0, 0) for output values.
 */
int gsp_insert_defs(gsp_t *p, const int *defs)
{
	unsigned mods, key;
	const int *dp;
	int rc;

	dp = defs;

	while (true) {
		/* Read the output values. */
		mods = *dp++;
		key = *dp++;
		if (key == 0)
			break;

		/* Insert one sequence. */
		rc = gsp_insert_seq(p, dp, mods, key);
		if (rc != 0)
			return rc;

		/* Skip to the next definition. */
		while (*dp != GSP_END)
			++dp;
		++dp;
	}

	return 0;
}

/** Insert one sequence into the parser.
 *
 * @param p	The parser.
 * @param seq	Sequence of input values terminated with GSP_END.
 * @param mods	Corresponsing output value.
 * @param key	Corresponsing output value.
 */
int gsp_insert_seq(gsp_t *p, const int *seq, unsigned mods, unsigned key)
{
	int state;
	gsp_trans_t *t;

	state = 0;
	t = NULL;

	/* Input sequence must be non-empty. */
	if (*seq == GSP_END)
		return -1;

	while (*(seq + 1) != GSP_END) {
		t = trans_lookup(p, state, *seq);
		if (t == NULL) {
			/* Create new state. */
			t = trans_new();
			t->old_state = state;
			t->input = *seq;
			t->new_state = p->states++;

			t->out_mods = 0;
			t->out_key = 0;

			trans_insert(p, t);
		}
		state = t->new_state;
		++seq;
	}

	/* Process the last transition. */
	t = trans_lookup(p, state, *seq);
	if (t != NULL) {
		exit(1);
		return -1;	/* Conflicting definition. */
	}

	t = trans_new();
	t->old_state = state;
	t->input = *seq;
	t->new_state = 0;

	t->out_mods = mods;
	t->out_key = key;

	trans_insert(p, t);

	return 0;
}

/** Compute one parser step.
 *
 * Computes the next state and output values for a given state and input.
 * This handles everything including restarts and default branches.
 *
 * @param p		The parser.
 * @param state		Old state.
 * @param input		Input symbol (scancode).
 * @param mods		Output value (modifier).
 * @param key		Output value (key).
 * @return		New state.
 */
int gsp_step(gsp_t *p, int state, int input, unsigned *mods, unsigned *key)
{
	gsp_trans_t *t;

	t = trans_lookup(p, state, input);
	if (t == NULL) {
		t = trans_lookup(p, state, GSP_DEFAULT);
	}

	if (t == NULL) {
		printf("gsp_step: not found, state=%d, input=0x%x\n",
		    state, input);
		*mods = 0;
		*key = 0;
		return 0;
	}

	*mods = t->out_mods;
	*key = t->out_key;

	return t->new_state;
}

/** Transition function lookup.
 *
 * Returns the value of the transition function for the given state
 * and input. Note that the transition must be specified precisely,
 * to obtain the default branch use input = GSP_DEFAULT.
 *
 * @param p		Parser.
 * @param state		Current state.
 * @param input		Input value.
 * @return		The transition or @c NULL if not defined.
 */
static gsp_trans_t *trans_lookup(gsp_t *p, int state, int input)
{
	ht_link_t *item;

	trans_key_t key = {
		.input = input,
		.old_state = state
	};

	item = hash_table_find(&p->trans, &key);
	if (item == NULL)
		return NULL;

	return hash_table_get_inst(item, gsp_trans_t, link);
}

/** Define a new transition.
 *
 * @param p	The parser.
 * @param t	Transition with all fields defined.
 */
static void trans_insert(gsp_t *p, gsp_trans_t *t)
{
	hash_table_insert(&p->trans, &t->link);
}

/** Allocate transition structure. */
static gsp_trans_t *trans_new(void)
{
	gsp_trans_t *t;

	t = malloc(sizeof(gsp_trans_t));
	if (t == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return t;
}


/**
 * @}
 */
