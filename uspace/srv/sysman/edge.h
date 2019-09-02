/*
 * Copyright (c) 2015 Michal Koutny
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
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

#ifndef SYSMAN_EDGE_H
#define SYSMAN_EDGE_H

#include <adt/list.h>

#include "unit.h"

/** Dependency edge between units in dependency graph
 *
 * @code
 * input ---> output
 * @endcode
 *
 */
typedef struct {
	/** Link to edges_out list */
	link_t edges_in;
	/** Link to edges_out list */
	link_t edges_out;

	bool commited;

	/** Unit that depends on another */
	unit_t *input;

	/** Unit that is dependency for another */
	unit_t *output;

	/** Name of the output unit, for resolved edges it's NULL
	 *
	 * @note Either output or output_nameis set. Never both nor none.
	 */
	char *output_name;
} unit_edge_t;

extern unit_edge_t *edge_create(void);
extern void edge_destroy(unit_edge_t **);

extern int edge_sprout_out(unit_t *, const char *);
extern void edge_resolve_output(unit_edge_t *, unit_t *);

extern int edge_connect(unit_t *, unit_t *);
extern void edge_remove(unit_edge_t **);

#endif
