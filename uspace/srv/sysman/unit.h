#ifndef SYSMAN_UNIT_H
#define SYSMAN_UNIT_H

#include <adt/list.h>

#include "unit_mnt.h"
#include "unit_cfg.h"

typedef enum {
	UNIT_TARGET = 0,
	UNIT_MOUNT,
	UNIT_CONFIGURATION
} unit_type_t;

typedef enum {
	STATE_EMBRYO = 0,
	STATE_STOPPED
} unit_state_t;

typedef struct {
	link_t units;

	unit_type_t type;
	unit_state_t state;

	list_t dependencies;
	list_t dependants;

	union {
		unit_mnt_t mnt;
		unit_cfg_t cfg;
	} data;
} unit_t;


extern unit_t *unit_create(unit_type_t);
extern void unit_destroy(unit_t **);


#endif
