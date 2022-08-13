/*
 * SPDX-FileCopyrightText: 2012 Sean Bartell
 * SPDX-FileCopyrightText: 2012 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BITHENGE_COMMON_H_
#define BITHENGE_COMMON_H_

#ifdef __HELENOS__
#include "helenos/common.h"
#else
#include "linux/common.h"
#endif

#ifdef BITHENGE_FAILURE_ENABLE
#include "failure.h"
#else
static inline errno_t bithenge_should_fail(void)
{
	return 0;
}
#endif

#endif
