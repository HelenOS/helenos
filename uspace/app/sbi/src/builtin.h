/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BUILTIN_H_
#define BUILTIN_H_

#include "mytypes.h"

void builtin_declare(stree_program_t *program);
void builtin_bind(builtin_t *bi);
void builtin_code_snippet(builtin_t *bi, const char *snippet);

stree_csi_t *builtin_get_gf_class(builtin_t *builtin);
void builtin_run_proc(run_t *run, stree_proc_t *proc);

rdata_var_t *builtin_get_self_mbr_var(run_t *run, const char *mbr_name);
void builtin_return_string(run_t *run, const char *str);

stree_symbol_t *builtin_declare_fun(stree_csi_t *csi, const char *name);
void builtin_fun_add_arg(stree_symbol_t *fun_sym, const char *name);

stree_symbol_t *builtin_find_lvl0(builtin_t *bi, const char *sym_name);
stree_symbol_t *builtin_find_lvl1(builtin_t *bi, const char *csi_name,
    const char *sym_name);

void builtin_fun_bind(builtin_t *bi, const char *csi_name,
    const char *sym_name, builtin_proc_t bproc);

#endif
