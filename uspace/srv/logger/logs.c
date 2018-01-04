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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include "logger.h"


static FIBRIL_MUTEX_INITIALIZE(log_list_guard);
static LIST_INITIALIZE(log_list);


static logger_log_t *find_log_by_name_and_parent_no_list_lock(const char *name, logger_log_t *parent)
{
	list_foreach(log_list, link, logger_log_t, log) {
		if ((parent == log->parent) && (str_cmp(log->name, name) == 0))
			return log;
	}

	return NULL;
}

static errno_t create_dest(const char *name, logger_dest_t **dest)
{
	logger_dest_t *result = malloc(sizeof(logger_dest_t));
	if (result == NULL)
		return ENOMEM;
	if (asprintf(&result->filename, "/log/%s", name) < 0) {
		free(result);
		return ENOMEM;
	}
	result->logfile = NULL;
	fibril_mutex_initialize(&result->guard);
	*dest = result;
	return EOK;
}

static logger_log_t *create_log_no_locking(const char *name, logger_log_t *parent)
{
	logger_log_t *result = calloc(1, sizeof(logger_log_t));
	if (result == NULL)
		return NULL;

	result->name = str_dup(name);
	if (result->name == NULL)
		goto error;

	/*
	 * Notice that we create new dest as the last
	 * operation that can fail and thus there is no code
	 * to deallocate dest.
	 */
	if (parent == NULL) {
		result->full_name = str_dup(name);
		if (result->full_name == NULL)
			goto error;
		errno_t rc = create_dest(name, &result->dest);
		if (rc != EOK)
			goto error;
	} else {
		if (asprintf(&result->full_name, "%s/%s",
		    parent->full_name, name) < 0)
			goto error;
		result->dest = parent->dest;
	}

	/* Following initializations cannot fail. */
	result->logged_level = LOG_LEVEL_USE_DEFAULT;
	fibril_mutex_initialize(&result->guard);
	link_initialize(&result->link);
	result->parent = parent;

	return result;

error:
	free(result->name);
	free(result->full_name);
	free(result);
	return NULL;

}

logger_log_t *find_or_create_log_and_lock(const char *name, sysarg_t parent_id)
{
	logger_log_t *result = NULL;
	logger_log_t *parent = (logger_log_t *) parent_id;

	fibril_mutex_lock(&log_list_guard);

	result = find_log_by_name_and_parent_no_list_lock(name, parent);
	if (result == NULL) {
		result = create_log_no_locking(name, parent);
		if (result == NULL)
			goto leave;
		list_append(&result->link, &log_list);
		if (result->parent != NULL) {
			fibril_mutex_lock(&result->parent->guard);
			result->parent->ref_counter++;
			fibril_mutex_unlock(&result->parent->guard);
		}
	}

	fibril_mutex_lock(&result->guard);

leave:
	fibril_mutex_unlock(&log_list_guard);

	return result;
}

logger_log_t *find_log_by_name_and_lock(const char *name)
{
	logger_log_t *result = NULL;

	fibril_mutex_lock(&log_list_guard);
	list_foreach(log_list, link, logger_log_t, log) {
		if (str_cmp(log->full_name, name) == 0) {
			fibril_mutex_lock(&log->guard);
			result = log;
			break;
		}
	}
	fibril_mutex_unlock(&log_list_guard);

	return result;
}

logger_log_t *find_log_by_id_and_lock(sysarg_t id)
{
	logger_log_t *result = NULL;

	fibril_mutex_lock(&log_list_guard);
	list_foreach(log_list, link, logger_log_t, log) {
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

void log_unlock(logger_log_t *log)
{
	assert(fibril_mutex_is_locked(&log->guard));
	fibril_mutex_unlock(&log->guard);
}

/** Decreases reference counter on the log and destory the log if
 * necessary.
 *
 * Precondition: log is locked.
 *
 * @param log Log to release from using by the caller.
 */
void log_release(logger_log_t *log)
{
	assert(fibril_mutex_is_locked(&log->guard));
	assert(log->ref_counter > 0);

	/* We are definitely not the last ones. */
	if (log->ref_counter > 1) {
		log->ref_counter--;
		fibril_mutex_unlock(&log->guard);
		return;
	}

	/*
	 * To prevent deadlock, we need to get the list lock first.
	 * Deadlock scenario:
	 * Us: LOCKED(log), want to LOCK(list)
	 * Someone else calls find_log_by_name_and_lock(log->fullname) ->
	 *   LOCKED(list), wants to LOCK(log)
	 */
	fibril_mutex_unlock(&log->guard);

	/* Ensuring correct locking order. */
	fibril_mutex_lock(&log_list_guard);
	/*
	 * The reference must be still valid because we have not decreased
	 * the reference counter.
	 */
	fibril_mutex_lock(&log->guard);
	assert(log->ref_counter > 0);
	log->ref_counter--;

	if (log->ref_counter > 0) {
		/*
		 * Meanwhile, someone else increased the ref counter.
		 * No big deal, we just return immediatelly.
		 */
		fibril_mutex_unlock(&log->guard);
		fibril_mutex_unlock(&log_list_guard);
		return;
	}

	/*
	 * Here we are on a destroy path. We need to
	 * - remove ourselves from the list
	 * - decrease reference of the parent (if not top-level log)
	 *   - we must do that after we relaase list lock to prevent
	 *     deadlock with ourselves
	 * - destroy dest (if top-level log)
	 */
	assert(log->ref_counter == 0);

	list_remove(&log->link);
	fibril_mutex_unlock(&log_list_guard);
	fibril_mutex_unlock(&log->guard);

	if (log->parent == NULL) {
		/*
		 * Due to lazy file opening in write_to_log(),
		 * it is possible that no file was actually opened.
		 */
		if (log->dest->logfile != NULL) {
			fclose(log->dest->logfile);
		}
		free(log->dest->filename);
		free(log->dest);
	} else {
		fibril_mutex_lock(&log->parent->guard);
		log_release(log->parent);
	}

	logger_log("Destroyed log %s.\n", log->full_name);

	free(log->name);
	free(log->full_name);

	free(log);
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

void registered_logs_init(logger_registered_logs_t *logs)
{
	logs->logs_count = 0;
}

bool register_log(logger_registered_logs_t *logs, logger_log_t *new_log)
{
	if (logs->logs_count >= MAX_REFERENCED_LOGS_PER_CLIENT) {
		return false;
	}

	assert(fibril_mutex_is_locked(&new_log->guard));
	new_log->ref_counter++;

	logs->logs[logs->logs_count] = new_log;
	logs->logs_count++;

	return true;
}

void unregister_logs(logger_registered_logs_t *logs)
{
	for (size_t i = 0; i < logs->logs_count; i++) {
		logger_log_t *log = logs->logs[i];
		fibril_mutex_lock(&log->guard);
		log_release(log);
	}
}

/**
 * @}
 */
