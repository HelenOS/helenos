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

/** @file Int builtin binding. */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../bigint.h"
#include "../builtin.h"
#include "../debug.h"
#include "../mytypes.h"
#include "../os/os.h"
#include "../rdata.h"
#include "../run.h"
#include "../strtab.h"

#include "bi_int.h"

static void bi_int_get_as_string(run_t *run);

/** Declare Int builtin.
 *
 * @param bi	Builtin object
 */
void bi_int_declare(builtin_t *bi)
{
	(void) bi;
}

/** Bind Int builtin.
 *
 * @param bi	Builtin object
 */
void bi_int_bind(builtin_t *bi)
{
	builtin_fun_bind(bi, "Int", "get_as_string", bi_int_get_as_string);
}

/** Return string representation.
 *
 * @param run	Runner object
 */
static void bi_int_get_as_string(run_t *run)
{
	rdata_var_t *self_value_var;
	bigint_t *ival;
	char *str;

	/* Extract self.Value */
	self_value_var = builtin_get_self_mbr_var(run, "Value");
	assert(self_value_var->vc == vc_int);
	ival = &self_value_var->u.int_v->value;

	bigint_get_as_string(ival, &str);

#ifdef DEBUG_RUN_TRACE
	printf("Convert int to string '%s'.\n", str);
#endif
	/* Ownership of str is transferred. */
	builtin_return_string(run, str);
}
