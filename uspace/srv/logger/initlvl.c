/*
 * Copyright (c) 2012 Vojtech Horky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
