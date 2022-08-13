/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RUN_T_H_
#define RUN_T_H_

#include "intmap_t.h"
#include "list_t.h"

/** Block activation record
 *
 * One block AR is created for each block that we enter. A variable declaration
 * statement inserts the variable here. Upon exiting the block we pop from the
 * stack, thus all the variables declared in that block are forgotten.
 */
typedef struct run_block_ar {
	/** Variables in this block */
	intmap_t vars; /* of rdata_var_t */
} run_block_ar_t;

/** Procedure activation record
 *
 * A procedure can be a member function, a named property or an indexed
 * property. A procedure activation record is created whenever a procedure
 * is invoked.
 */
typedef struct run_proc_ar {
	/** Object on which the procedure is being invoked or @c NULL. */
	struct rdata_var *obj;

	/** Procedure being invoked */
	struct stree_proc *proc;

	/** Block activation records */
	list_t block_ar; /* of run_block_ar_t */

	/** Procedure return value or @c NULL if not set. */
	struct rdata_item *retval;
} run_proc_ar_t;

/** Bailout mode
 *
 * Determines whether control is bailing out of a statement, function, etc.
 */
typedef enum {
	/** Normal execution */
	bm_none,

	/** Break from statement */
	bm_stat,

	/** Return from procedure */
	bm_proc,

	/** Exception */
	bm_exc,

	/** Unrecoverable runtime error */
	bm_error
} run_bailout_mode_t;

/** Thread activation record
 *
 * We can walk the list of function ARs to get a function call backtrace.
 */
typedef struct run_thread_ar {
	/** Function activation records */
	list_t proc_ar; /* of run_proc_ar_t */

	/** Bailout mode */
	run_bailout_mode_t bo_mode;

	/** Exception cspan */
	struct cspan *exc_cspan;

	/** Exception payload */
	struct rdata_value *exc_payload;

	/** @c b_true if a run-time error occured. */
	bool_t error;
} run_thread_ar_t;

/** Runner state object */
typedef struct run {
	/** Code of the program being executed */
	struct stree_program *program;

	/** Thread-private state */
	run_thread_ar_t *thread_ar;

	/** Global state */
	struct rdata_var *gdata;
} run_t;

#endif
