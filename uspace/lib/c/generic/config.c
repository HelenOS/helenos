/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <config.h>
#include <sysinfo.h>
#include <str.h>

bool config_key_exists(const char *key)
{
	char *value;
	bool exists;

	value = config_get_value(key);
	exists = (value != NULL);
	free(value);

	return exists;
}

char *config_get_value(const char *key)
{
	char *value = NULL;
	char *boot_args;
	size_t size;

	boot_args = sysinfo_get_data("boot_args", &size);
	if (!boot_args || !size) {
		/*
		 * No boot arguments, no value.
		 */
		return NULL;
	}

	char *args = boot_args;
	char *arg;
	while ((arg = str_tok(args, " ", &args)) != NULL) {
		arg = str_tok(arg, "=", &value);
		if (arg && !str_cmp(arg, key))
			break;
		else
			value = NULL;
	}

	if (value)
		value = str_dup(value);

	free(boot_args);

	return value;
}

/** @}
 */
