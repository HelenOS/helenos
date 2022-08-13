/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SYMBOL_H_
#define SYMBOL_H_

#include "mytypes.h"

stree_symbol_t *symbol_xlookup_in_csi(stree_program_t *prog,
    stree_csi_t *scope, stree_texpr_t *texpr);
stree_symbol_t *symbol_lookup_in_csi(stree_program_t *prog, stree_csi_t *scope,
    stree_ident_t *name);
stree_symbol_t *symbol_search_csi(stree_program_t *prog, stree_csi_t *scope,
    stree_ident_t *name);
stree_symbol_t *symbol_search_csi_no_base(stree_program_t *prog,
    stree_csi_t *scope, stree_ident_t *name);
stree_csi_t *symbol_get_base_class(stree_program_t *prog, stree_csi_t *csi);
stree_texpr_t *symbol_get_base_class_ref(stree_program_t *prog,
    stree_csi_t *csi);
stree_symbol_t *symbol_find_epoint(stree_program_t *prog, stree_ident_t *name);

stree_deleg_t *symbol_to_deleg(stree_symbol_t *symbol);
stree_symbol_t *deleg_to_symbol(stree_deleg_t *deleg);
stree_csi_t *symbol_to_csi(stree_symbol_t *symbol);
stree_symbol_t *csi_to_symbol(stree_csi_t *csi);
stree_ctor_t *symbol_to_ctor(stree_symbol_t *symbol);
stree_symbol_t *ctor_to_symbol(stree_ctor_t *ctor);
stree_enum_t *symbol_to_enum(stree_symbol_t *symbol);
stree_symbol_t *enum_to_symbol(stree_enum_t *enum_d);
stree_fun_t *symbol_to_fun(stree_symbol_t *symbol);
stree_symbol_t *fun_to_symbol(stree_fun_t *fun);
stree_var_t *symbol_to_var(stree_symbol_t *symbol);
stree_symbol_t *var_to_symbol(stree_var_t *var);
stree_prop_t *symbol_to_prop(stree_symbol_t *symbol);
stree_symbol_t *prop_to_symbol(stree_prop_t *prop);

stree_symbol_t *csimbr_to_symbol(stree_csimbr_t *csimbr);

void symbol_print_fqn(stree_symbol_t *symbol);

#endif
