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
 */

#ifndef SEARCH_H__
#define SEARCH_H__

#include <str.h>

struct search;
typedef struct search search_t;
typedef bool (*search_equals_fn)(const char32_t, const char32_t);
typedef errno_t (*search_producer_fn)(void *, char32_t *);
typedef errno_t (*search_mark_fn)(void *, void **);
typedef void (*search_mark_free_fn)(void *);

typedef struct match {
	aoff64_t length;
	void *end;
} match_t;

typedef struct search_ops {
	search_equals_fn equals;
	search_producer_fn producer;
	search_mark_fn mark;
	search_mark_free_fn mark_free;
} search_ops_t;

extern bool char_exact_equals(const char32_t, const char32_t);
extern search_t *search_init(const char *, void *, search_ops_t, bool);
extern errno_t search_next_match(search_t *, match_t *);
extern void search_fini(search_t *);

#endif

/** @}
 */
