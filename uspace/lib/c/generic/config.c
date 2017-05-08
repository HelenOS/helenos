/*
 * Copyright (c) 2016 Jakub Jermar
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
