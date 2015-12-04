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

#include <errno.h>

#include "unit.h"

static void unit_tgt_init(unit_t *unit)
{
	unit_tgt_t *u_tgt = CAST_TGT(unit);
	assert(u_tgt);
	/* Nothing more to do */
}

static void unit_tgt_destroy(unit_t *unit)
{
	unit_tgt_t *u_tgt = CAST_TGT(unit);
	assert(u_tgt);
	/* Nothing more to do */
}

static int unit_tgt_load(unit_t *unit, ini_configuration_t *ini_conf,
    text_parse_t *text_parse)
{
	unit_tgt_t *u_tgt = CAST_TGT(unit);
	assert(u_tgt);

	return EOK;
}

static int unit_tgt_start(unit_t *unit)
{
	unit_tgt_t *u_tgt = CAST_TGT(unit);
	assert(u_tgt);

	unit->state = STATE_STARTED;
	return EOK;
}

static int unit_tgt_stop(unit_t *unit)
{
	unit_tgt_t *u_tgt = CAST_TGT(unit);
	assert(u_tgt);

	unit->state = STATE_STOPPED;
	return EOK;
}

static void unit_tgt_exposee_created(unit_t *unit)
{
	/* Target has no exposees. */
	assert(false);
}

static void unit_tgt_fail(unit_t *unit)
{
}


DEFINE_UNIT_VMT(unit_tgt)

