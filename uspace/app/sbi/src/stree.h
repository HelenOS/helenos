/*
 * Copyright (c) 2011 Jiri Svoboda
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

#ifndef STREE_H_
#define STREE_H_

#include "mytypes.h"

stree_module_t *stree_module_new(void);
stree_modm_t *stree_modm_new(modm_class_t mc);
stree_csi_t *stree_csi_new(csi_class_t cc);
stree_csimbr_t *stree_csimbr_new(csimbr_class_t cc);
stree_ctor_t *stree_ctor_new(void);
stree_deleg_t *stree_deleg_new(void);
stree_enum_t *stree_enum_new(void);
stree_embr_t *stree_embr_new(void);
stree_fun_t *stree_fun_new(void);
stree_var_t *stree_var_new(void);
stree_prop_t *stree_prop_new(void);
stree_targ_t *stree_targ_new(void);

stree_symbol_attr_t *stree_symbol_attr_new(symbol_attr_class_t sac);

stree_proc_t *stree_proc_new(void);
stree_proc_arg_t *stree_proc_arg_new(void);
stree_fun_sig_t *stree_fun_sig_new(void);
stree_arg_attr_t *stree_arg_attr_new(arg_attr_class_t aac);

stree_stat_t *stree_stat_new(stat_class_t sc);
stree_vdecl_t *stree_vdecl_new(void);
stree_if_t *stree_if_new(void);
stree_switch_t *stree_switch_new(void);
stree_while_t *stree_while_new(void);
stree_for_t *stree_for_new(void);
stree_raise_t *stree_raise_new(void);
stree_break_t *stree_break_new(void);
stree_return_t *stree_return_new(void);
stree_wef_t *stree_wef_new(void);
stree_exps_t *stree_exps_new(void);

stree_except_t *stree_except_new(void);
stree_if_clause_t *stree_if_clause_new(void);
stree_when_t *stree_when_new(void);
stree_block_t *stree_block_new(void);

stree_expr_t *stree_expr_new(expr_class_t ec);
stree_assign_t *stree_assign_new(assign_class_t ac);
stree_binop_t *stree_binop_new(binop_class_t bc);
stree_unop_t *stree_unop_new(unop_class_t uc);
stree_new_t *stree_new_new(void);
stree_access_t *stree_access_new(void);
stree_call_t *stree_call_new(void);
stree_index_t *stree_index_new(void);
stree_as_t *stree_as_new(void);
stree_box_t *stree_box_new(void);
stree_nameref_t *stree_nameref_new(void);

stree_ident_t *stree_ident_new(void);
stree_literal_t *stree_literal_new(literal_class_t ltc);
stree_self_ref_t *stree_self_ref_new(void);

stree_texpr_t *stree_texpr_new(texpr_class_t tc);
stree_taccess_t *stree_taccess_new(void);
stree_tapply_t *stree_tapply_new(void);
stree_tindex_t *stree_tindex_new(void);
stree_tliteral_t *stree_tliteral_new(tliteral_class_t tlc);
stree_tnameref_t *stree_tnameref_new(void);

stree_symbol_t *stree_symbol_new(symbol_class_t sc);
stree_program_t *stree_program_new(void);

bool_t stree_symbol_has_attr(stree_symbol_t *symbol, symbol_attr_class_t sac);
bool_t stree_arg_has_attr(stree_proc_arg_t *arg, arg_attr_class_t aac);
bool_t stree_is_csi_derived_from_csi(stree_csi_t *a, stree_csi_t *b);
bool_t stree_symbol_is_static(stree_symbol_t *symbol);
stree_targ_t *stree_csi_find_targ(stree_csi_t *csi, stree_ident_t *ident);
stree_embr_t *stree_enum_find_mbr(stree_enum_t *enum_d, stree_ident_t *ident);
stree_ident_t *stree_csimbr_get_name(stree_csimbr_t *csimbr);

#endif
