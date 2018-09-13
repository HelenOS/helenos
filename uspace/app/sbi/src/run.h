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

#ifndef RUN_H_
#define RUN_H_

#include "mytypes.h"

void run_init(run_t *run);
void run_gdata_init(run_t *run);
void run_program(run_t *run, stree_program_t *prog);
void run_proc(run_t *run, run_proc_ar_t *proc_ar, rdata_item_t **res);
void run_stat(run_t *run, stree_stat_t *stat, rdata_item_t **res);

void run_print_fun_bt(run_t *run);

void run_exc_check_unhandled(run_t *run);
void run_raise_error(run_t *run);
rdata_item_t *run_recovery_item(run_t *run);

rdata_var_t *run_local_vars_lookup(run_t *run, sid_t name);
run_proc_ar_t *run_get_current_proc_ar(run_t *run);
run_block_ar_t *run_get_current_block_ar(run_t *run);
stree_csi_t *run_get_current_csi(run_t *run);

rdata_var_t *run_sobject_get(run_t *run, stree_csi_t *pcsi,
    rdata_var_t *pobj_var, sid_t name);
rdata_var_t *run_sobject_find(run_t *run, stree_csi_t *csi);
rdata_var_t *run_fun_sobject_find(run_t *run, stree_fun_t *fun);

void run_value_item_to_var(rdata_item_t *item, rdata_var_t **var);
void run_proc_ar_set_args(run_t *run, run_proc_ar_t *proc_ar,
    list_t *arg_vals);
void run_proc_ar_set_setter_arg(run_t *run, run_proc_ar_t *proc_ar,
    rdata_item_t *arg_val);
void run_proc_ar_create(run_t *run, rdata_var_t *obj, stree_proc_t *proc,
    run_proc_ar_t **rproc_ar);
void run_proc_ar_destroy(run_t *run, run_proc_ar_t *proc_ar);

var_class_t run_item_get_vc(run_t *run, rdata_item_t *item);
void run_cvt_value_item(run_t *run, rdata_item_t *item, rdata_item_t **ritem);
void run_address_read(run_t *run, rdata_address_t *address,
    rdata_item_t **ritem);
void run_address_write(run_t *run, rdata_address_t *address,
    rdata_value_t *value);
void run_reference(run_t *run, rdata_var_t *var, rdata_item_t **res);
void run_dereference(run_t *run, rdata_item_t *ref, cspan_t *cspan,
    rdata_item_t **ritem);

void run_raise_exc(run_t *run, stree_csi_t *csi, cspan_t *cspan);
bool_t run_is_bo(run_t *run);

void run_var_new(run_t *run, tdata_item_t *ti, rdata_var_t **rvar);

run_thread_ar_t *run_thread_ar_new(void);
run_proc_ar_t *run_proc_ar_new(void);
void run_proc_ar_delete(run_proc_ar_t *proc_ar);
run_block_ar_t *run_block_ar_new(void);
void run_block_ar_delete(run_block_ar_t *block_ar);
void run_proc_ar_delete(run_proc_ar_t *proc_ar);
void run_block_ar_destroy(run_t *run, run_block_ar_t *block_ar);

#endif
