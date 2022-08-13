/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MYTYPES_H_
#define MYTYPES_H_

/** Boolean type compatible with builtin C 'boolean' operators. */
typedef enum {
	b_false = 0,
	b_true = 1
} bool_t;

/** Node state for walks. */
typedef enum {
	ws_unvisited,
	ws_active,
	ws_visited
} walk_state_t;

/** Static vs. nonstatic */
typedef enum {
	sn_nonstatic,
	sn_static
} statns_t;

/** Error return codes. */
#include <errno.h>
/** We need NULL defined. */
#include <stddef.h>

#ifndef EOK
#define EOK 0
#endif

#include "bigint_t.h"
#include "builtin_t.h"
#include "cspan_t.h"
#include "input_t.h"
#include "intmap_t.h"
#include "lex_t.h"
#include "list_t.h"
#include "parse_t.h"
#include "rdata_t.h"
#include "run_t.h"
#include "stree_t.h"
#include "strtab_t.h"
#include "stype_t.h"
#include "tdata_t.h"

#endif
