#ifndef SYSMAN_CONFIGURATION_H
#define SYSMAN_CONFIGURATION_H

#include "unit.h"

extern void configuration_init(void);

extern int configuration_add_unit(unit_t *);

extern void configuration_start_update(void);

extern void configuration_commit(void);

extern void configuration_rollback(void);

extern int configuration_resolve_dependecies(void);

extern unit_t *configuration_find_unit_by_name(const char *);


#endif


