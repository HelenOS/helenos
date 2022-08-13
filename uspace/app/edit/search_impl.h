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

#ifndef SEARCH_IMPL_H__
#define SEARCH_IMPL_H__

#include "search.h"

/** Search state */
struct search {
	/* Note: This structure is opaque for the user. */

	char32_t *pattern;
	size_t pattern_length;
	ssize_t *back_table;
	size_t pattern_pos;
	void *client_data;
	search_ops_t ops;
};

#endif

/** @}
 */
