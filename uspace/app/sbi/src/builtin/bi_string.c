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

/** @file String builtin binding. */

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

#include "bi_string.h"

static void bi_string_length(run_t *run);
static void bi_string_slice(run_t *run);

/** Declare String builtin.
 *
 * @param bi	Builtin object
 */
void bi_string_declare(builtin_t *bi)
{
	(void) bi;
}

/** Bind String builtin.
 *
 * @param bi	Builtin object
 */
void bi_string_bind(builtin_t *bi)
{
	builtin_fun_bind(bi, "String", "get_length", bi_string_length);
	builtin_fun_bind(bi, "String", "Slice", bi_string_slice);
}

/** Return length of the string.
 *
 * @param run	Runner object
 */
static void bi_string_length(run_t *run)
{
	rdata_var_t *self_value_var;
	const char *str;
	size_t str_l;

	rdata_int_t *rint;
	rdata_var_t *rvar;
	rdata_value_t *rval;
	rdata_item_t *ritem;

	run_proc_ar_t *proc_ar;

	/* Extract self.Value */
	self_value_var = builtin_get_self_mbr_var(run, "Value");
	assert(self_value_var->vc == vc_string);
	str = self_value_var->u.string_v->value;
	str_l = os_str_length(str);

#ifdef DEBUG_RUN_TRACE
	printf("Get length of string '%s'.\n", str);
#endif

	/* Construct return value. */
	rint = rdata_int_new();
	bigint_init(&rint->value, (int) str_l);

	rvar = rdata_var_new(vc_int);
	rvar->u.int_v = rint;
	rval = rdata_value_new();
	rval->var = rvar;

	ritem = rdata_item_new(ic_value);
	ritem->u.value = rval;

	proc_ar = run_get_current_proc_ar(run);
	proc_ar->retval = ritem;
}

/** Return slice (substring) of the string.
 *
 * @param run	Runner object
 */
static void bi_string_slice(run_t *run)
{
	rdata_var_t *self_value_var;
	const char *str;
	const char *slice;
	size_t str_l;

	rdata_var_t *start_var;
	int start;

	rdata_var_t *length_var;
	int length;

	errno_t rc;

	/* Extract self.Value */
	self_value_var = builtin_get_self_mbr_var(run, "Value");
	assert(self_value_var->vc == vc_string);
	str = self_value_var->u.string_v->value;
	str_l = os_str_length(str);

	/* Get argument @a start. */
	start_var = run_local_vars_lookup(run, strtab_get_sid("start"));
	assert(start_var);
	assert(start_var->vc == vc_int);

	rc = bigint_get_value_int(&start_var->u.int_v->value, &start);
	if (rc != EOK || start < 0 || (size_t) start > str_l) {
		printf("Error: Parameter 'start' to Slice() out of range.\n");
		exit(1);
	}

	/* Get argument @a length. */
	length_var = run_local_vars_lookup(run, strtab_get_sid("length"));
	assert(length_var);
	assert(length_var->vc == vc_int);

	rc = bigint_get_value_int(&length_var->u.int_v->value, &length);
	if (rc != EOK || length < 0 || (size_t) (start + length) > str_l) {
		printf("Error: Parameter 'length' to Slice() out of range.\n");
		exit(1);
	}

#ifdef DEBUG_RUN_TRACE
	printf("Construct Slice(%d, %d) from string '%s'.\n",
	    start, length, str);
#endif
	slice = os_str_aslice(str, start, length);

	/* Ownership of slice is transferred. */
	builtin_return_string(run, slice);
}
