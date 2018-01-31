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

#ifndef STYPE_H_
#define STYPE_H_

#include "mytypes.h"

void stype_module(stype_t *stype, stree_module_t *module);
void stype_ctor_header(stype_t *stype, stree_ctor_t *ctor);
void stype_deleg(stype_t *stype, stree_deleg_t *deleg);
void stype_enum(stype_t *stype, stree_enum_t *enum_d);
void stype_fun_header(stype_t *stype, stree_fun_t *fun);
void stype_prop_header(stype_t *stype, stree_prop_t *prop);
void stype_stat(stype_t *stype, stree_stat_t *stat, bool_t want_value);

void stype_note_error(stype_t *stype);
tdata_item_t *stype_recovery_titem(stype_t *stype);

stree_expr_t *stype_convert(stype_t *stype, stree_expr_t *expr,
    tdata_item_t *dest);
void stype_convert_failure(stype_t *stype, stype_conv_class_t convc,
    stree_expr_t *expr, tdata_item_t *dest);
stree_expr_t *stype_box_expr(stype_t *stype, stree_expr_t *expr);
tdata_item_t *stype_tobject_find_pred(stype_t *stype, tdata_item_t *src,
    tdata_item_t *dest);
errno_t stype_targs_check_equal(stype_t *stype, tdata_item_t *a_ti,
    tdata_item_t *b_ti);

tdata_fun_sig_t *stype_deleg_get_sig(stype_t *stype, tdata_deleg_t *tdeleg);

void stype_titem_to_tvv(stype_t *stype, tdata_item_t *ti, tdata_tvv_t **rtvv);

tdata_item_t *stype_boolean_titem(stype_t *stype);

stree_vdecl_t *stype_local_vars_lookup(stype_t *stype, sid_t name);
stree_proc_arg_t *stype_proc_args_lookup(stype_t *stype, sid_t name);
stype_block_vr_t *stype_get_current_block_vr(stype_t *stype);

stype_proc_vr_t *stype_proc_vr_new(void);
stype_block_vr_t *stype_block_vr_new(void);

#endif
