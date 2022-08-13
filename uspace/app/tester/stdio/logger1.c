/*
 * SPDX-FileCopyrightText: 2012 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <io/log.h>
#include "../tester.h"

const char *test_logger1(void)
{
	for (log_level_t level = 0; level < LVL_LIMIT; level++) {
		log_msg(LOG_DEFAULT, level, "Testing logger, level %d.", (int) level);
	}

	return NULL;
}
