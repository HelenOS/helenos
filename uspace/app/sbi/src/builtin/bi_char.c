/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file Char builtin binding. */

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

#include "bi_char.h"

static void bi_char_get_as_string(run_t *run);

/** Declare Char builtin.
 *
 * @param bi	Builtin object
 */
void bi_char_declare(builtin_t *bi)
{
	(void) bi;
}

/** Bind Char builtin.
 *
 * @param bi	Builtin object
 */
void bi_char_bind(builtin_t *bi)
{
	builtin_fun_bind(bi, "Char", "get_as_string", bi_char_get_as_string);
}

/** Return string representation.
 *
 * @param run	Runner object
 */
static void bi_char_get_as_string(run_t *run)
{
	rdata_var_t *self_value_var;
	bigint_t *cval;
	char *str;
	int char_val;
	errno_t rc;

#ifdef DEBUG_RUN_TRACE
	printf("Convert char to string.\n");
#endif
	/* Extract self.Value */
	self_value_var = builtin_get_self_mbr_var(run, "Value");
	assert(self_value_var->vc == vc_char);
	cval = &self_value_var->u.char_v->value;

	rc = bigint_get_value_int(cval, &char_val);
	if (rc != EOK) {
		/* XXX Should raise exception. */
		builtin_return_string(run, os_str_dup("?"));
		return;
	}

	str = os_chr_to_astr((char32_t) char_val);

	/* Ownership of str is transferred. */
	builtin_return_string(run, str);
}
