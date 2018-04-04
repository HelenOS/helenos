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

/** @file Builtin symbol binding.
 *
 * 'Builtin' symbols are implemented outside of the language itself.
 * Here we refer to entities residing within the interpreted universe
 * as 'internal', while anything implemented outside this universe
 * as 'external'. This module facilitates declaration of builtin
 * symbols and the binding of these symbols to their external
 * implementation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "builtin/bi_boxed.h"
#include "builtin/bi_error.h"
#include "builtin/bi_char.h"
#include "builtin/bi_console.h"
#include "builtin/bi_int.h"
#include "builtin/bi_task.h"
#include "builtin/bi_textfile.h"
#include "builtin/bi_string.h"
#include "input.h"
#include "intmap.h"
#include "lex.h"
#include "list.h"
#include "mytypes.h"
#include "os/os.h"
#include "parse.h"
#include "rdata.h"
#include "run.h"
#include "stree.h"
#include "strtab.h"
#include "symbol.h"

#include "builtin.h"

static builtin_t *builtin_new(void);

/** Declare builtin symbols in the program.
 *
 * Declares symbols that will be hooked to builtin interpreter procedures.
 *
 * @param program	Program in which to declare builtin symbols.
 */
void builtin_declare(stree_program_t *program)
{
	builtin_t *bi;

	bi = builtin_new();
	bi->program = program;
	program->builtin = bi;

	/*
	 * Declare grandfather class.
	 */

	builtin_code_snippet(bi,
	    "class Object is\n"
	    "end\n");
	bi->gf_class = builtin_find_lvl0(bi, "Object");

	/*
	 * Declare other builtin classes/functions.
	 */

	bi_error_declare(bi);
	bi_char_declare(bi);
	bi_console_declare(bi);
	bi_int_declare(bi);
	bi_task_declare(bi);
	bi_textfile_declare(bi);
	bi_string_declare(bi);
}

/** Bind internal interpreter references to symbols in the program.
 *
 * This is performed in separate phase for several reasons. First,
 * symbol lookups do not work until ancestry is processed. Second,
 * this gives a chance to process the library first and thus bind
 * to symbols defined there.
 */
void builtin_bind(builtin_t *bi)
{
	bi_boxed_bind(bi);
	bi_error_bind(bi);
	bi_char_bind(bi);
	bi_console_bind(bi);
	bi_int_bind(bi);
	bi_task_bind(bi);
	bi_textfile_bind(bi);
	bi_string_bind(bi);
}

/** Get grandfather class.
 *
 * Grandfather class is the class from which all other classes are
 * (directly or indirectly) derived.
 *
 * @param builtin	Builtin context (corresponsds to program).
 * @return		Grandfather class (CSI).
 */
stree_csi_t *builtin_get_gf_class(builtin_t *builtin)
{
	if (builtin->gf_class == NULL)
		return NULL;

	return symbol_to_csi(builtin->gf_class);
}

/** Allocate new builtin context object.
 *
 * @return	Builtin context object.
 */
static builtin_t *builtin_new(void)
{
	builtin_t *builtin;

	builtin = calloc(1, sizeof(builtin_t));
	if (builtin == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return builtin;
}

/** Parse a declaration code snippet.
 *
 * Parses a piece of code from a string at the module level. This can be
 * used to declare builtin symbols easily and without need for an external
 * file.
 */
void builtin_code_snippet(builtin_t *bi, const char *snippet)
{
	input_t *input;
	lex_t lex;
	parse_t parse;

	input_new_string(&input, snippet);
	lex_init(&lex, input);
	parse_init(&parse, bi->program, &lex);
	parse_module(&parse);
}

/** Simplifed search for a global symbol.
 *
 * The specified symbol must exist.
 *
 * @param bi		Builtin context object.
 * @param sym_name	Name of symbol to find.
 * @return		Symbol.
 */
stree_symbol_t *builtin_find_lvl0(builtin_t *bi, const char *sym_name)
{
	stree_symbol_t *sym;
	stree_ident_t *ident;

	ident = stree_ident_new();

	ident->sid = strtab_get_sid(sym_name);
	sym = symbol_lookup_in_csi(bi->program, NULL, ident);
	assert(sym != NULL);

	return sym;
}

/** Simplifed search for a level 1 symbol.
 *
 * The specified symbol must exist.
 *
 * @param bi		Builtin context object.
 * @param csi_name	CSI in which to look for symbol.
 * @param sym_name	Name of symbol to find.
 * @return		Symbol.
 */
stree_symbol_t *builtin_find_lvl1(builtin_t *bi, const char *csi_name,
    const char *sym_name)
{
	stree_symbol_t *csi_sym;
	stree_csi_t *csi;

	stree_symbol_t *mbr_sym;
	stree_ident_t *ident;

	ident = stree_ident_new();

	ident->sid = strtab_get_sid(csi_name);
	csi_sym = symbol_lookup_in_csi(bi->program, NULL, ident);
	assert(csi_sym != NULL);
	csi = symbol_to_csi(csi_sym);
	assert(csi != NULL);

	ident->sid = strtab_get_sid(sym_name);
	mbr_sym = symbol_lookup_in_csi(bi->program, csi, ident);
	assert(mbr_sym != NULL);

	return mbr_sym;
}

/** Bind level 1 member function to external implementation.
 *
 * Binds a member function (of a global class) to external implementation.
 * The specified CSI and member function must exist.
 *
 * @param bi		Builtin context object.
 * @param csi_name	CSI which contains the function.
 * @param sym_name	Function name.
 * @param bproc		Pointer to C function implementation.
 */
void builtin_fun_bind(builtin_t *bi, const char *csi_name,
    const char *sym_name, builtin_proc_t bproc)
{
	stree_symbol_t *fun_sym;
	stree_fun_t *fun;

	fun_sym = builtin_find_lvl1(bi, csi_name, sym_name);
	assert(fun_sym != NULL);
	fun = symbol_to_fun(fun_sym);
	assert(fun != NULL);

	fun->proc->bi_handler = bproc;
}

/** Execute a builtin procedure.
 *
 * Executes a procedure that has an external implementation.
 *
 * @param run		Runner object.
 * @param proc		Procedure that has an external implementation.
 */
void builtin_run_proc(run_t *run, stree_proc_t *proc)
{
	stree_symbol_t *fun_sym;
	builtin_proc_t bproc;

#ifdef DEBUG_RUN_TRACE
	printf("Run builtin procedure.\n");
#endif
	fun_sym = proc->outer_symbol;

	bproc = proc->bi_handler;
	if (bproc == NULL) {
		printf("Error: Unrecognized builtin function '");
		symbol_print_fqn(fun_sym);
		printf("'.\n");
		exit(1);
	}

	/* Run the builtin procedure handler. */
	(*bproc)(run);
}

/** Get pointer to member var of current object.
 *
 * Returns the var node that corresponds to a member of the currently
 * active object with the given name. This member must exist.
 *
 * @param run		Runner object.
 * @param mbr_name	Name of member to find.
 * @return		Var node of the member.
 */
rdata_var_t *builtin_get_self_mbr_var(run_t *run, const char *mbr_name)
{
	run_proc_ar_t *proc_ar;
	rdata_object_t *object;
	sid_t mbr_name_sid;
	rdata_var_t *mbr_var;

	proc_ar = run_get_current_proc_ar(run);
	assert(proc_ar->obj->vc == vc_object);
	object = proc_ar->obj->u.object_v;

	mbr_name_sid = strtab_get_sid(mbr_name);
	mbr_var = intmap_get(&object->fields, mbr_name_sid);
	assert(mbr_var != NULL);

	return mbr_var;
}

/** Return string value from builtin procedure.
 *
 * Makes it easy for a builtin procedure to return value of type @c string.
 *
 * @param run		Runner object
 * @param str		String value. Must be allocated from heap and its
 *			ownership is hereby given up.
 */
void builtin_return_string(run_t *run, const char *astr)
{
	rdata_string_t *rstring;
	rdata_var_t *rvar;
	rdata_value_t *rval;
	rdata_item_t *ritem;

	run_proc_ar_t *proc_ar;

#ifdef DEBUG_RUN_TRACE
	printf("Return string '%s' from builtin function.\n", astr);
#endif
	rstring = rdata_string_new();
	rstring->value = astr;

	rvar = rdata_var_new(vc_string);
	rvar->u.string_v = rstring;
	rval = rdata_value_new();
	rval->var = rvar;

	ritem = rdata_item_new(ic_value);
	ritem->u.value = rval;

	proc_ar = run_get_current_proc_ar(run);
	proc_ar->retval = ritem;
}

/** Declare a static builtin function in @a csi.
 *
 * Declare a builtin function member of CSI @a csi. Deprecated in favor
 * of builtin_code_snippet().
 *
 * @param csi		CSI in which to declare function.
 * @param name		Name of member function to declare.
 * @return		Symbol of newly declared function.
 */
stree_symbol_t *builtin_declare_fun(stree_csi_t *csi, const char *name)
{
	stree_ident_t *ident;
	stree_fun_t *fun;
	stree_fun_sig_t *sig;
	stree_csimbr_t *csimbr;
	stree_symbol_t *fun_sym;
	stree_symbol_attr_t *sym_attr;

	ident = stree_ident_new();
	ident->sid = strtab_get_sid(name);

	fun = stree_fun_new();
	fun->name = ident;
	fun->proc = stree_proc_new();
	fun->proc->body = NULL;
	sig = stree_fun_sig_new();
	fun->sig = sig;

	list_init(&fun->sig->args);

	csimbr = stree_csimbr_new(csimbr_fun);
	csimbr->u.fun = fun;

	fun_sym = stree_symbol_new(sc_fun);
	fun_sym->u.fun = fun;
	fun_sym->outer_csi = csi;

	sym_attr = stree_symbol_attr_new(sac_static);
	list_append(&fun_sym->attr, sym_attr);

	fun->symbol = fun_sym;
	fun->proc->outer_symbol = fun_sym;

	list_append(&csi->members, csimbr);

	return fun_sym;
}

/** Add one formal parameter to function.
 *
 * Used to incrementally construct formal parameter list of a builtin
 * function. Deprecated in favor of builtin_code_snippet(). Does not
 * support type checking.
 *
 * @param fun_sym	Symbol of function to add parameters to.
 * @param name		Name of parameter to add.
 */
void builtin_fun_add_arg(stree_symbol_t *fun_sym, const char *name)
{
	stree_proc_arg_t *proc_arg;
	stree_fun_t *fun;

	fun = symbol_to_fun(fun_sym);
	assert(fun != NULL);

	proc_arg = stree_proc_arg_new();
	proc_arg->name = stree_ident_new();
	proc_arg->name->sid = strtab_get_sid(name);
	proc_arg->type = NULL; /* XXX */

	list_append(&fun->sig->args, proc_arg);
}
