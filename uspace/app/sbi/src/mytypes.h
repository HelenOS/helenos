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
