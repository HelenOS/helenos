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

#include <assert.h>

#include "../edge.h"

#include "mock_unit.h"

unit_t *mock_units[MAX_TYPES][MAX_UNITS];

unit_type_t unit_types[] = {
	UNIT_CONFIGURATION,
	UNIT_MOUNT,
	UNIT_SERVICE,
	UNIT_TARGET
};

void mock_create_units(void)
{
	for (int i = 0; i < MAX_TYPES; ++i) {
		unit_type_t type = unit_types[i];
		for (int j = 0; j < MAX_UNITS; ++j) {
			mock_units[type][j] = unit_create(type);
			assert(mock_units[type][j] != NULL);

			asprintf(&mock_units[type][j]->name, "%u_%u", type, j);
			assert(mock_units[type][j]->name != NULL);
		}
	}
}

void mock_destroy_units(void)
{
	for (int i = 0; i < MAX_TYPES; ++i) {
		unit_type_t type = unit_types[i];
		for (int j = 0; j < MAX_UNITS; ++j) {
			unit_destroy(&mock_units[type][j]);
		}
	}
}

void mock_set_units_state(unit_state_t state)
{
	for (int i = 0; i < MAX_TYPES; ++i) {
		unit_type_t type = unit_types[i];
		for (int j = 0; j < MAX_UNITS; ++j) {
			mock_units[type][j]->state = state;
		}
	}
}

void mock_add_edge(unit_t *input, unit_t *output)
{
	int rc = edge_connect(input, output);
	assert(rc == EOK);

	link_t *link = list_last(&input->edges_out);
	unit_edge_t *e =
	    list_get_instance(link, unit_edge_t, edges_out);
	e->commited = true;
}

int mock_unit_vmt_start_sync(unit_t *unit)
{
	unit->state = STATE_STARTED;
	return EOK;
}

int mock_unit_vmt_start_async(unit_t *unit)
{
	unit->state = STATE_STARTING;
	return EOK;
}

void mock_unit_vmt_exposee_created(unit_t *unit)
{
	unit->state = STATE_STARTED;
	unit_notify_state(unit);
}

