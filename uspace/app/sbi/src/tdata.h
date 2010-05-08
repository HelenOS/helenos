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

#ifndef TDATA_H_
#define TDATA_H_

#include "mytypes.h"

tdata_item_t *tdata_item_new(titem_class_t tic);
tdata_array_t *tdata_array_new(void);
tdata_object_t *tdata_object_new(void);
tdata_primitive_t *tdata_primitive_new(tprimitive_class_t tpc);
tdata_deleg_t *tdata_deleg_new(void);
tdata_ebase_t *tdata_ebase_new(void);
tdata_enum_t *tdata_enum_new(void);
tdata_fun_t *tdata_fun_new(void);
tdata_vref_t *tdata_vref_new(void);

tdata_fun_sig_t *tdata_fun_sig_new(void);

tdata_tvv_t *tdata_tvv_new(void);
tdata_item_t *tdata_tvv_get_val(tdata_tvv_t *tvv, sid_t name);
void tdata_tvv_set_val(tdata_tvv_t *tvv, sid_t name, tdata_item_t *tvalue);

bool_t tdata_is_csi_derived_from_ti(stree_csi_t *a, tdata_item_t *tb);
bool_t tdata_is_ti_derived_from_ti(tdata_item_t *ta, tdata_item_t *tb);
bool_t tdata_item_equal(tdata_item_t *a, tdata_item_t *b);

void tdata_item_subst(tdata_item_t *ti, tdata_tvv_t *tvv, tdata_item_t **res);
void tdata_item_print(tdata_item_t *titem);

#endif
