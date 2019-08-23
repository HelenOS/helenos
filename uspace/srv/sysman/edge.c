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

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <str.h>

#include "edge.h"

static void edge_init(unit_edge_t *e)
{
	memset(e, 0, sizeof(*e));
	link_initialize(&e->edges_in);
	link_initialize(&e->edges_out);
}

static unit_edge_t *edge_extract_internal(unit_t *input, unit_t *output)
{
	list_foreach(input->edges_out, edges_out, unit_edge_t, e) {
		if (e->output == output) {
			return e;
		}
	}

	return NULL;
}

unit_edge_t *edge_create(void)
{
	unit_edge_t *e = malloc(sizeof(unit_edge_t));
	if (e) {
		edge_init(e);
	}
	return e;
}

void edge_destroy(unit_edge_t **e_ptr)
{
	unit_edge_t *e = *e_ptr;
	if (e == NULL) {
		return;
	}

	list_remove(&e->edges_in);
	list_remove(&e->edges_out);

	free(e->output_name);
	free(e);

	*e_ptr = NULL;
}

int edge_sprout_out(unit_t *input, const char *output_name)
{
	int rc;
	unit_edge_t *e = edge_create();

	if (e == NULL) {
		rc = ENOMEM;
		goto finish;
	}

	//TODO check multi-edges
	e->output_name = str_dup(output_name);
	if (e->output_name == NULL) {
		rc = ENOMEM;
		goto finish;
	}

	list_append(&e->edges_out, &input->edges_out);
	e->input = input;

	rc = EOK;

finish:
	if (rc != EOK) {
		edge_destroy(&e);
	}
	return rc;
}

void edge_resolve_output(unit_edge_t *e, unit_t *output)
{
	assert(e->output == NULL);
	assert(e->output_name != NULL);

	e->output = output;
	list_append(&e->edges_in, &output->edges_in);

	free(e->output_name);
	e->output_name = NULL;
}


/**
 * @return        EOK on success
 * @return        ENOMEM
 * @return        EEXIST
 */
int edge_connect(unit_t *input, unit_t *output)
{
	if (edge_extract_internal(input, output)) {
		return EEXIST;
	}

	unit_edge_t *e = edge_create();
	if (e == NULL) {
		return ENOMEM;
	}

	list_append(&e->edges_in, &output->edges_in);
	list_append(&e->edges_out, &input->edges_out);

	e->input = input;
	e->output = output;
	return EOK;
}

/** Remove edge from dependency graph
 *
 * Given edge is removed from graph and unallocated.
 */
void edge_remove(unit_edge_t **e_ptr)
{
	/*
	 * So far it's just passing, however, edge_destroy is considered
	 * low-level and edge_remove could later e.g. support transactions.
	 */
	edge_destroy(e_ptr);
}
