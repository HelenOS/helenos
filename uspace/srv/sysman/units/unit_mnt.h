#ifndef SYSMAN_UNIT_MNT_H
#define SYSMAN_UNIT_MNT_H

#include "unit_types.h"

typedef struct {
	const char *type;
	const char *mountpoint;
	const char *device;
} unit_mnt_t;

extern unit_ops_t unit_mnt_ops;


#endif

