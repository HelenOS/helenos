/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PARSE_H_
#define PARSE_H_

#include "mytypes.h"

void parse_init(parse_t *parse, struct stree_program *prog, struct lex *lex);
void parse_module(parse_t *parse);

stree_stat_t *parse_stat(parse_t *parse);
stree_ident_t *parse_ident(parse_t *parse);

void parse_raise_error(parse_t *parse);
void parse_note_error(parse_t *parse);
bool_t parse_is_error(parse_t *parse);
void parse_recover_error(parse_t *parse);

/*
 * Parsing primitives
 */
lem_t *lcur(parse_t *parse);
lclass_t lcur_lc(parse_t *parse);
cspan_t *lcur_span(parse_t *parse);
cspan_t *lprev_span(parse_t *parse);

void lskip(parse_t *parse);
void lcheck(parse_t *parse, lclass_t lc);
void lmatch(parse_t *parse, lclass_t lc);
void lunexpected_error(parse_t *parse);

bool_t terminates_block(lclass_t lclass);

#endif
