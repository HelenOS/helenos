/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RUN_TEXPR_H_
#define RUN_TEXPR_H_

#include "mytypes.h"

void run_texpr(stree_program_t *prog, stree_csi_t *ctx, stree_texpr_t *texpr,
    tdata_item_t **res);

#endif
