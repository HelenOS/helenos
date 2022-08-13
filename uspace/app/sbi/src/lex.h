/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LEX_H_
#define LEX_H_

#include "mytypes.h"

void lclass_print(lclass_t lclass);
void lem_print(lem_t *lem);
void lem_print_coords(lem_t *lem);

void lex_init(lex_t *lex, struct input *input);
void lex_next(lex_t *lex);
lem_t *lex_get_current(lex_t *lex);
lem_t *lex_peek_prev(lex_t *lex);

#endif
