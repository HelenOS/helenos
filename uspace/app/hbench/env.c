/*
 * Copyright (c) 2019 Vojtech Horky
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

/** @addtogroup hbench
 * @{
 */
/**
 * @file
 */

#include <stdlib.h>
#include <stdio.h>
#include <str.h>
#include "hbench.h"

typedef struct {
	ht_link_t link;

	char *key;
	char *value;
} param_t;

static size_t param_hash(const ht_link_t *item)
{
	param_t *param = hash_table_get_inst(item, param_t, link);
	return str_size(param->key);
}

static size_t param_key_hash(const void *key)
{
	const char *key_str = key;
	return str_size(key_str);
}

static bool param_key_equal(const void *key, const ht_link_t *item)
{
	param_t *param = hash_table_get_inst(item, param_t, link);
	const char *key_str = key;

	return str_cmp(param->key, key_str) == 0;
}

static bool param_equal(const ht_link_t *link_a, const ht_link_t *link_b)
{
	param_t *a = hash_table_get_inst(link_a, param_t, link);
	param_t *b = hash_table_get_inst(link_b, param_t, link);

	return str_cmp(a->key, b->key) == 0;
}

static void param_remove(ht_link_t *item)
{
	param_t *param = hash_table_get_inst(item, param_t, link);
	free(param->key);
	free(param->value);
}

static const hash_table_ops_t param_hash_table_ops = {
	.hash = param_hash,
	.key_hash = param_key_hash,
	.key_equal = param_key_equal,
	.equal = param_equal,
	.remove_callback = param_remove
};

errno_t bench_env_init(bench_env_t *env)
{
	bool ok = hash_table_create(&env->parameters, 0, 0, &param_hash_table_ops);
	if (!ok) {
		return ENOMEM;
	}

	env->run_count = DEFAULT_RUN_COUNT;
	env->minimal_run_duration_nanos = MSEC2NSEC(DEFAULT_MIN_RUN_DURATION_MSEC);

	return EOK;
}

void bench_env_cleanup(bench_env_t *env)
{
	hash_table_destroy(&env->parameters);
}

errno_t bench_env_param_set(bench_env_t *env, const char *key, const char *value)
{
	param_t *param = malloc(sizeof(param_t));
	if (param == NULL) {
		return ENOMEM;
	}

	param->key = str_dup(key);
	param->value = str_dup(value);

	if ((param->key == NULL) || (param->value == NULL)) {
		free(param->key);
		free(param->value);
		free(param);

		return ENOMEM;
	}

	hash_table_insert(&env->parameters, &param->link);

	return EOK;
}

const char *bench_env_param_get(bench_env_t *env, const char *key, const char *default_value)
{
	ht_link_t *item = hash_table_find(&env->parameters, (char *) key);

	if (item == NULL) {
		return default_value;
	}

	param_t *param = hash_table_get_inst(item, param_t, link);
	return param->value;
}

/** @}
 */
