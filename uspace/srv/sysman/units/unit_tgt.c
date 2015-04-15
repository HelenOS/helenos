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

	return EOK;
}

DEFINE_UNIT_VMT(unit_tgt)

