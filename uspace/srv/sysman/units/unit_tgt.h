#ifndef SYSMAN_UNIT_TGT_H
#define SYSMAN_UNIT_TGT_H

#include "unit.h"

typedef struct {
	unit_t unit;
} unit_tgt_t;

extern unit_vmt_t unit_tgt_ops;

#endif
