/*
 * Copyright (c) 2010 Jiri Svoboda
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
