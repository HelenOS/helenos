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

#ifndef STREE_H_
#define STREE_H_

#include "mytypes.h"

stree_module_t *stree_module_new(void);
stree_modm_t *stree_modm_new(modm_class_t mc);
stree_csi_t *stree_csi_new(csi_class_t cc);
stree_csimbr_t *stree_csimbr_new(csimbr_class_t cc);
stree_fun_t *stree_fun_new(void);
stree_var_t *stree_var_new(void);
stree_prop_t *stree_prop_new(void);

stree_fun_arg_t *stree_fun_arg_new(void);

stree_stat_t *stree_stat_new(stat_class_t sc);
stree_vdecl_t *stree_vdecl_new(void);
stree_if_t *stree_if_new(void);
stree_while_t *stree_while_new(void);
stree_for_t *stree_for_new(void);
stree_raise_t *stree_raise_new(void);
stree_wef_t *stree_wef_new(void);
stree_exps_t *stree_exps_new(void);
stree_block_t *stree_block_new(void);

stree_expr_t *stree_expr_new(expr_class_t ec);
stree_assign_t *stree_assign_new(assign_class_t ac);
stree_binop_t *stree_binop_new(binop_class_t bc);
stree_access_t *stree_access_new(void);
stree_call_t *stree_call_new(void);
stree_nameref_t *stree_nameref_new(void);

stree_ident_t *stree_ident_new(void);
stree_literal_t *stree_literal_new(literal_class_t ltc);

stree_texpr_t *stree_texpr_new(texpr_class_t tc);
stree_tapply_t *stree_tapply_new(void);
stree_taccess_t *stree_taccess_new(void);
stree_tnameref_t *stree_tnameref_new(void);

stree_symbol_t *stree_symbol_new(symbol_class_t sc);
stree_program_t *stree_program_new(void);

#endif
