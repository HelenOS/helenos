/*
 * SPDX-FileCopyrightText: 2012 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <offset.h>

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
