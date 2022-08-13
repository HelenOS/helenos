/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file Console builtin binding. */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../bigint.h"
#include "../builtin.h"
#include "../list.h"
#include "../mytypes.h"
#include "../run.h"
#include "../stree.h"
#include "../strtab.h"
#include "../symbol.h"

#include "bi_console.h"

static void bi_console_write(run_t *run);
static void bi_console_writeline(run_t *run);

/** Declare Console builtin.
 *
 * @param bi	Builtin object
 */
void bi_console_declare(builtin_t *bi)
{
	stree_modm_t *modm;
	stree_csi_t *csi;
	stree_ident_t *ident;
	stree_symbol_t *symbol;
	stree_symbol_t *fun_sym;

	/* Declare class Builtin */

	ident = stree_ident_new();
	ident->sid = strtab_get_sid("Console");

	csi = stree_csi_new(csi_class);
	csi->name = ident;
	list_init(&csi->targ);
	list_init(&csi->members);

	modm = stree_modm_new(mc_csi);
	modm->u.csi = csi;

	symbol = stree_symbol_new(sc_csi);
	symbol->u.csi = csi;
	symbol->outer_csi = NULL;
	csi->symbol = symbol;

	list_append(&bi->program->module->members, modm);

	/* Declare Builtin.Write(). */

	fun_sym = builtin_declare_fun(csi, "Write");
	builtin_fun_add_arg(fun_sym, "arg");

	/* Declare Builtin.WriteLine(). */

	fun_sym = builtin_declare_fun(csi, "WriteLine");
	builtin_fun_add_arg(fun_sym, "arg");
}

/** Bind builtin functions.
 *
 * @param bi	Builtin object
 */
void bi_console_bind(builtin_t *bi)
{
	builtin_fun_bind(bi, "Console", "Write", bi_console_write);
	builtin_fun_bind(bi, "Console", "WriteLine", bi_console_writeline);
}

/** Write to the console.
 *
 * @param run	Runner object
 */
static void bi_console_write(run_t *run)
{
	rdata_var_t *var;
	int char_val;
	errno_t rc;

#ifdef DEBUG_RUN_TRACE
	printf("Called Console.Write()\n");
#endif
	var = run_local_vars_lookup(run, strtab_get_sid("arg"));
	assert(var);

	switch (var->vc) {
	case vc_bool:
		printf("%s", var->u.bool_v->value ? "true" : "false");
		break;
	case vc_char:
		rc = bigint_get_value_int(&var->u.char_v->value, &char_val);
		if (rc == EOK)
			printf("%lc", char_val);
		else
			printf("???");
		break;
	case vc_int:
		bigint_print(&var->u.int_v->value);
		break;
	case vc_string:
		printf("%s", var->u.string_v->value);
		break;
	default:
		printf("Unimplemented: Write() with unsupported type.\n");
		exit(1);
	}
}

/** Write a line of output.
 *
 * @param run	Runner object
 */
static void bi_console_writeline(run_t *run)
{
#ifdef DEBUG_RUN_TRACE
	printf("Called Console.WriteLine()\n");
#endif
	bi_console_write(run);
	putchar('\n');
}
