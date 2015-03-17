#ifndef SYSMAN_UNIT_CFG_H
#define SYSMAN_UNIT_CFG_H

#include "unit_types.h"

typedef struct {
	const char *path;
} unit_cfg_t;

extern unit_ops_t unit_cfg_ops;

#endif
