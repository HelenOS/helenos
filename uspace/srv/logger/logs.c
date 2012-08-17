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

/** @addtogroup logger
 * @{
 */
#include <assert.h>
#include <malloc.h>
#include <str.h>
#include <stdio.h>
#include <errno.h>
#include "logger.h"


static FIBRIL_MUTEX_INITIALIZE(log_list_guard);
static LIST_INITIALIZE(log_list);


static logger_log_t *find_log_by_name_and_parent_no_list_lock_and_acquire(const char *name, logger_log_t *parent)
{
	list_foreach(log_list, it) {
		logger_log_t *log = list_get_instance(it, logger_log_t, link);
		if ((parent == log->parent) && (str_cmp(log->name, name) == 0)) {
			fibril_mutex_lock(&log->guard);
			return log;
		}
	}

	return NULL;
}

static int create_dest(const char *name, logger_dest_t **dest)
{
	logger_dest_t *result = malloc(sizeof(logger_dest_t));
	if (result == NULL)
		return ENOMEM;
	int rc = asprintf(&result->filename, "/log/%s", name);
	if (rc < 0) {
		free(result);
		return ENOMEM;
	}
	result->logfile = NULL;
	fibril_mutex_initialize(&result->guard);
	*dest = result;
	return EOK;
}

int find_or_create_log_and_acquire(const char *name, sysarg_t parent_id, logger_log_t **log_out)
{
	int rc;
	logger_log_t *result = NULL;
	logger_log_t *parent = (logger_log_t *) parent_id;

	fibril_mutex_lock(&log_list_guard);

	result = find_log_by_name_and_parent_no_list_lock_and_acquire(name, parent);
	if (result != NULL) {
		rc = EOK;
		goto leave;
	}

	result = calloc(1, sizeof(logger_log_t));
	if (result == NULL) {
		rc = ENOMEM;
		goto leave;
	}

	result->logged_level = LOG_LEVEL_USE_DEFAULT;
	result->name = str_dup(name);
	if (parent == NULL) {
		result->full_name = str_dup(name);
		rc = create_dest(name, &result->dest);
		if (rc != EOK)
			goto error_result_allocated;
	} else {
		rc = asprintf(&result->full_name, "%s/%s",
		    parent->full_name, name);
		if (rc < 0)
			goto error_result_allocated;
		result->dest = parent->dest;
	}
	result->parent = parent;
	fibril_mutex_initialize(&result->guard);

	link_initialize(&result->link);

	fibril_mutex_lock(&result->guard);

	list_append(&result->link, &log_list);


leave:
	fibril_mutex_unlock(&log_list_guard);

	if (rc == EOK) {
		assert(fibril_mutex_is_locked(&result->guard));
		*log_out = result;
	}

	return rc;

error_result_allocated:
	free(result->name);
	free(result->full_name);
	free(result);

	fibril_mutex_unlock(&log_list_guard);

	return rc;
}

logger_log_t *find_log_by_name_and_acquire(const char *name)
{
	logger_log_t *result = NULL;

	fibril_mutex_lock(&log_list_guard);
	list_foreach(log_list, it) {
		logger_log_t *log = list_get_instance(it, logger_log_t, link);
		if (str_cmp(log->full_name, name) == 0) {
			fibril_mutex_lock(&log->guard);
			result = log;
			break;
		}
	}
	fibril_mutex_unlock(&log_list_guard);

	return result;
}

logger_log_t *find_log_by_id_and_acquire(sysarg_t id)
{
	logger_log_t *result = NULL;

	fibril_mutex_lock(&log_list_guard);
	list_foreach(log_list, it) {
		logger_log_t *log = list_get_instance(it, logger_log_t, link);
		if ((sysarg_t) log == id) {
			fibril_mutex_lock(&log->guard);
			result = log;
			break;
		}
	}
	fibril_mutex_unlock(&log_list_guard);

	return result;
}

static log_level_t get_actual_log_level(logger_log_t *log)
{
	/* Find recursively proper log level. */
	if (log->logged_level == LOG_LEVEL_USE_DEFAULT) {
		if (log->parent == NULL)
			return get_default_logging_level();
		else
			return get_actual_log_level(log->parent);
	}
	return log->logged_level;
}

bool shall_log_message(logger_log_t *log, log_level_t level)
{
	fibril_mutex_lock(&log_list_guard);
	bool result = level <= get_actual_log_level(log);
	fibril_mutex_unlock(&log_list_guard);
	return result;
}

void log_release(logger_log_t *log)
{
	assert(fibril_mutex_is_locked(&log->guard));
	fibril_mutex_unlock(&log->guard);
}

void write_to_log(logger_log_t *log, log_level_t level, const char *message)
{
	assert(fibril_mutex_is_locked(&log->guard));
	assert(log->dest != NULL);
	fibril_mutex_lock(&log->dest->guard);
	if (log->dest->logfile == NULL)
		log->dest->logfile = fopen(log->dest->filename, "a");

	if (log->dest->logfile != NULL) {
		fprintf(log->dest->logfile, "[%s] %s: %s\n",
		    log->full_name, log_level_str(level),
		    (const char *) message);
		fflush(log->dest->logfile);
	}

	fibril_mutex_unlock(&log->dest->guard);
}

/**
 * @}
 */
