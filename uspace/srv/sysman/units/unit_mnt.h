#ifndef SYSMAN_UNIT_MNT_H
#define SYSMAN_UNIT_MNT_H

#include "unit.h"

typedef struct {
	unit_t unit;

	char *type;
	char *mountpoint;
	char *device;
} unit_mnt_t;

extern unit_vmt_t unit_mnt_ops;

#endif

