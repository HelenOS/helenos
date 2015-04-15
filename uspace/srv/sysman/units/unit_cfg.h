#ifndef SYSMAN_UNIT_CFG_H
#define SYSMAN_UNIT_CFG_H

#include "unit.h"

typedef struct {
	unit_t unit;

	char *path;
} unit_cfg_t;

extern unit_vmt_t unit_cfg_ops;

#endif
