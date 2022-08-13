/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
