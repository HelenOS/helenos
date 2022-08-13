/*
 * SPDX-FileCopyrightText: 2012 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @addtogroup logger
 * @{
 */

/** @file
 */

#include <errno.h>
#include "logger.h"

log_level_t default_logging_level = LVL_NOTE;
static FIBRIL_MUTEX_INITIALIZE(default_logging_level_guard);

log_level_t get_default_logging_level(void)
{
	fibril_mutex_lock(&default_logging_level_guard);
	log_level_t result = default_logging_level;
	fibril_mutex_unlock(&default_logging_level_guard);
	return result;
}

errno_t set_default_logging_level(log_level_t new_level)
{
	if (new_level >= LVL_LIMIT)
		return ERANGE;
	fibril_mutex_lock(&default_logging_level_guard);
	default_logging_level = new_level;
	fibril_mutex_unlock(&default_logging_level_guard);
	return EOK;
}

/**
 * @}
 */
