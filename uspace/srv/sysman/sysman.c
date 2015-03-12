#include <errno.h>

#include "sysman.h"

int sysman_unit_start(unit_t *unit)
{
	// satisfy dependencies
	// start unit (via restarter)
	return EOK;
}
