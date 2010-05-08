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
