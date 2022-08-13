/*
 * SPDX-FileCopyrightText: 2017 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */

#include "internal/common.h"
#include <dlfcn.h>

int dlclose(void *handle)
{
	not_implemented();
	return 0;
}

char *dlerror(void)
{
	not_implemented();
	return (char *)"dlerror()";
}

/** @}
 */
