/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CSPAN_H_
#define CSPAN_H_

#include "mytypes.h"

cspan_t *cspan_new(input_t *input, int line0, int col0, int line1, int col1);
cspan_t *cspan_merge(cspan_t *a, cspan_t *b);
void cspan_print(cspan_t *cspan);

#endif
