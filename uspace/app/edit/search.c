/*
 * Copyright (c) 2012 Martin Sucha
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

/** @addtogroup edit
 * @{
 */
/**
 * @file
 * @brief Simple searching facility.
 */

#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <types/common.h>

#include "search.h"
#include "search_impl.h"

search_t *search_init(const char *pattern, void *client_data, search_ops_t ops,
    bool reverse)
{
	search_t *search = calloc(1, sizeof(search_t));
	if (search == NULL)
		return NULL;
	
	wchar_t *p = str_to_awstr(pattern);
	if (p == NULL) {
		free(search);
		return NULL;
	}
	
	search->pattern_length = wstr_length(p);
	
	if (reverse) {
		/* Reverse the pattern */
		size_t pos, half;
		half = search->pattern_length / 2;
		for (pos = 0; pos < half; pos++) {
			wchar_t tmp = p[pos];
			p[pos] = p[search->pattern_length - pos - 1];
			p[search->pattern_length - pos - 1] = tmp;
		}
	}
	
	search->pattern = p;
	
	search->client_data = client_data;
	search->ops = ops;
	search->back_table = calloc(search->pattern_length, sizeof(ssize_t));
	if (search->back_table == NULL) {
		free(search->pattern);
		free(search);
		return NULL;
	}
	
	search->pattern_pos = 0;
	
	search->back_table[0] = -1;
	search->back_table[1] = 0;
	size_t table_idx = 2;
	size_t pattern_idx = 0;
	while (table_idx < search->pattern_length) {
		if (ops.equals(search->pattern[table_idx - 1],
		    search->pattern[pattern_idx])) {
			pattern_idx++;
			search->back_table[table_idx] = pattern_idx;
			table_idx++;
		}
		else if (pattern_idx > 0) {
			pattern_idx = search->back_table[pattern_idx];
		}
		else {
			pattern_idx = 0;
			table_idx++;
		}
	}
	
	return search;
}

errno_t search_next_match(search_t *s, match_t *match)
{
	search_equals_fn eq = s->ops.equals;
	
	wchar_t cur_char;
	errno_t rc = EOK;
	while ((rc = s->ops.producer(s->client_data, &cur_char)) == EOK && cur_char > 0) {
		/* Deal with mismatches */
		while (s->pattern_pos > 0 && !eq(cur_char, s->pattern[s->pattern_pos])) {
			s->pattern_pos = s->back_table[s->pattern_pos];
		}
		/* Check if the character matched */
		if (eq(cur_char, s->pattern[s->pattern_pos])) {
			s->pattern_pos++;
			if (s->pattern_pos == s->pattern_length) {
				s->pattern_pos = s->back_table[s->pattern_pos];
				rc = s->ops.mark(s->client_data, &match->end);
				if (rc != EOK)
					return rc;
				match->length = s->pattern_length;
				return EOK;
			}
		}
	}
	
	match->end = NULL;
	match->length = 0;
	
	return rc;
}

void search_fini(search_t *search)
{
	free(search->pattern);
	free(search->back_table);
	
}

bool char_exact_equals(const wchar_t a, const wchar_t b)
{
	return a == b;
}

/** @}
 */
