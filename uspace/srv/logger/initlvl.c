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
#include <str_error.h>
#include <sysinfo.h>
#include <str.h>
#include <stdlib.h>
#include "logger.h"

static void parse_single_level_setting(char *setting)
{
	char *tmp;
	char *key = str_tok(setting, "=", &tmp);
	char *value = str_tok(tmp, "=", &tmp);
	if (key == NULL)
		return;
	if (value == NULL) {
		log_level_t level;
		errno_t rc = log_level_from_str(key, &level);
		if (rc != EOK)
			return;
		set_default_logging_level(level);
		return;
	}

	log_level_t level;
	errno_t rc = log_level_from_str(value, &level);
	if (rc != EOK)
		return;

	logger_log_t *log = find_or_create_log_and_lock(key, 0);
	if (log == NULL)
		return;

	log->logged_level = level;
	log->ref_counter++;

	log_unlock(log);
}

void parse_level_settings(char *settings)
{
	char *tmp;
	char *single_setting = str_tok(settings, " ", &tmp);
	while (single_setting != NULL) {
		parse_single_level_setting(single_setting);
		single_setting = str_tok(tmp, " ", &tmp);
	}
}

void parse_initial_settings(void)
{
	size_t argument_size;
	void *argument = sysinfo_get_data("init_args.logger", &argument_size);
	if (argument == NULL)
		return;

	char level_str[200];
	str_cpy(level_str, 200, (const char *) argument);
	free(argument);

	parse_level_settings(level_str);
}

/**
 * @}
 */
