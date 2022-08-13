/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file Error classes (used with exception handling). */

#include <assert.h>
#include "../builtin.h"
#include "../mytypes.h"
#include "../symbol.h"

#include "bi_error.h"

/** Declare error class hierarchy.
 *
 * @param bi	Builtin object
 */
void bi_error_declare(builtin_t *bi)
{
	/*
	 * Declare class Error and its subclasses.
	 * Here, class Error supplants a package or namespace.
	 */

	builtin_code_snippet(bi,
	    "class Error is\n"
	    "    -- Common ancestor of all error classes\n"
	    "    class Base is\n"
	    "    end\n"
	    "    -- Accessing nil reference\n"
	    "    class NilReference : Base is\n"
	    "    end\n"
	    "    -- Array index out of bounds\n"
	    "    class OutOfBounds : Base is\n"
	    "    end\n"
	    "end\n");
}

/** Bind error class hierarchy.
 *
 * @param bi	Builtin object
 */
void bi_error_bind(builtin_t *bi)
{
	stree_symbol_t *sym;

	/* Declare class Error and its subclasses. */

	sym = builtin_find_lvl1(bi, "Error", "OutOfBounds");
	bi->error_outofbounds = symbol_to_csi(sym);
	assert(bi->error_outofbounds != NULL);

	sym = builtin_find_lvl1(bi, "Error", "NilReference");
	bi->error_nilreference = symbol_to_csi(sym);
	assert(bi->error_nilreference != NULL);
}
