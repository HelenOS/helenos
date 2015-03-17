#ifndef SYSMAN_UNIT_H
#define SYSMAN_UNIT_H

#include <adt/list.h>
#include <fibril_synch.h>

#include "unit_mnt.h"
#include "unit_cfg.h"
#include "unit_tgt.h"
#include "unit_types.h"

struct unit {
	link_t units;

	unit_type_t type;

	unit_state_t state;
	fibril_mutex_t state_mtx;
	fibril_condvar_t state_cv;

	list_t dependencies;
	list_t dependants;

	union {
		unit_mnt_t mnt;
		unit_cfg_t cfg;
	} data;
};


extern unit_t *unit_create(unit_type_t);
extern void unit_destroy(unit_t **);

// TODO add flags argument with explicit notification?
extern void unit_set_state(unit_t *, unit_state_t);

extern int unit_start(unit_t *);

#endif
