/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RUN_EXPR_H_
#define RUN_EXPR_H_

#include "mytypes.h"

void run_expr(run_t *run, stree_expr_t *expr, rdata_item_t **res);

void run_new_csi_inst_ref(run_t *run, stree_csi_t *csi, statns_t sn,
    rdata_item_t **res);
void run_new_csi_inst(run_t *run, stree_csi_t *csi, statns_t sn,
    rdata_var_t **res);

void run_equal(run_t *run, rdata_value_t *v1, rdata_value_t *v2, bool_t *res);
bool_t run_item_boolean_value(run_t *run, rdata_item_t *item);

#endif
