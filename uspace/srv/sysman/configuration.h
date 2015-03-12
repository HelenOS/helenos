#ifndef SYSMAN_CONFIGURATION_H
#define SYSMAN_CONFIGURATION_H

#include "unit.h"

extern void configuration_init(void);

extern int configuration_add_unit(unit_t *);

extern int configuration_commit(void);

#endif


