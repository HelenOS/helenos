/*
 * SPDX-FileCopyrightText: 2012 Sean Bartell
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup bithenge
 * @{
 */
/**
 * @file
 * Write a tree as JSON or other text formats.
 */

#ifndef BITHENGE_PRINT_H_
#define BITHENGE_PRINT_H_

#include "tree.h"

/** Specifies the format to be used when printing. */
typedef enum {
	/** Print a Python value. Note that internal nodes will be represented
	 * as unordered dictionaries.
	 */
	BITHENGE_PRINT_PYTHON,
	/** Print JSON. Due to the limitations of JSON, type information may be
	 * lost.
	 */
	BITHENGE_PRINT_JSON,
} bithenge_print_type_t;

errno_t bithenge_print_node(bithenge_print_type_t, bithenge_node_t *);
errno_t bithenge_print_node_to_string(char **, size_t *, bithenge_print_type_t,
    bithenge_node_t *);

#endif

/** @}
 */
