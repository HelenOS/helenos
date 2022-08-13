/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BUILTIN_T_H_
#define BUILTIN_T_H_

#include "run_t.h"

/** Builtin symbols object
 *
 * Aggregates references to builtin symbols.
 */
typedef struct builtin {
	/** Containing program object */
	struct stree_program *program;

	/** Grandfather object */
	struct stree_symbol *gf_class;

	/** Boxed variants of primitive types */
	struct stree_symbol *boxed_bool;
	struct stree_symbol *boxed_char;
	struct stree_symbol *boxed_int;
	struct stree_symbol *boxed_string;

	/** Error class for nil reference access */
	struct stree_csi *error_nilreference;

	/** Error class for out-of-bounds array access */
	struct stree_csi *error_outofbounds;
} builtin_t;

/** Callback to run for a builtin procedure */
typedef void (*builtin_proc_t)(run_t *);

#endif
