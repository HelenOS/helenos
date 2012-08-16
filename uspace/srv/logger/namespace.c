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

/** @file
 * Logging namespaces.
 */


struct logging_namespace {
	fibril_mutex_t guard;
	size_t writers_count;
	fibril_condvar_t level_changed_cv;
	FILE *logfile;
	log_level_t level;
	const char *name;

	link_t link;
};

static FIBRIL_MUTEX_INITIALIZE(namespace_list_guard);
static LIST_INITIALIZE(namespace_list);

static log_level_t namespace_get_actual_log_level(logging_namespace_t *namespace)
{
	fibril_mutex_lock(&namespace->guard);
	log_level_t level = namespace->level;
	fibril_mutex_unlock(&namespace->guard);

	if (level == LOG_LEVEL_USE_DEFAULT)
		level = get_default_logging_level();

	return level;
}

static logging_namespace_t *namespace_find_no_lock(const char *name)
{
	list_foreach(namespace_list, it) {
		logging_namespace_t *namespace = list_get_instance(it, logging_namespace_t, link);
		if (str_cmp(namespace->name, name) == 0) {
			return namespace;
		}
	}

	return NULL;
}

static logging_namespace_t *namespace_create_no_lock(const char *name)
{
	logging_namespace_t *existing = namespace_find_no_lock(name);
	if (existing != NULL) {
		return NULL;
	}

	logging_namespace_t *namespace = malloc(sizeof(logging_namespace_t));
	if (namespace == NULL) {
		return NULL;
	}

	namespace->name = str_dup(name);
	if (namespace->name == NULL) {
		free(namespace);
		return NULL;
	}

	char *logfilename;
	int rc = asprintf(&logfilename, "/log/%s", name);
	if (rc < 0) {
		free(namespace->name);
		free(namespace);
		return NULL;
	}
	namespace->logfile = fopen(logfilename, "a");
	free(logfilename);
	if (namespace->logfile == NULL) {
		free(namespace->name);
		free(namespace);
		return NULL;
	}

	namespace->level = LOG_LEVEL_USE_DEFAULT;

	fibril_mutex_initialize(&namespace->guard);
	fibril_condvar_initialize(&namespace->level_changed_cv);
	namespace->writers_count = 0;
	link_initialize(&namespace->link);

	list_append(&namespace->link, &namespace_list);

	return namespace;
}


logging_namespace_t *namespace_create(const char *name)
{
	fibril_mutex_lock(&namespace_list_guard);
	logging_namespace_t *result = namespace_create_no_lock(name);
	fibril_mutex_unlock(&namespace_list_guard);
	return result;
}

const char *namespace_get_name(logging_namespace_t *namespace)
{
	assert(namespace);
	return namespace->name;
}

static void namespace_destroy_careful(logging_namespace_t *namespace)
{
	assert(namespace);
	fibril_mutex_lock(&namespace_list_guard);

	fibril_mutex_lock(&namespace->guard);
	if (namespace->writers_count > 0) {
		fibril_mutex_unlock(&namespace->guard);
		fibril_mutex_unlock(&namespace_list_guard);
		return;
	}

	list_remove(&namespace->link);

	fibril_mutex_unlock(&namespace->guard);
	fibril_mutex_unlock(&namespace_list_guard);

	// TODO - destroy pending messages
	fclose(namespace->logfile);
	free(namespace->name);
	free(namespace);
}

void namespace_destroy(logging_namespace_t *namespace)
{
	namespace_destroy_careful(namespace);
}

logging_namespace_t *namespace_writer_attach(const char *name)
{
	logging_namespace_t *namespace = NULL;

	fibril_mutex_lock(&namespace_list_guard);

	namespace = namespace_find_no_lock(name);

	if (namespace == NULL) {
		namespace = namespace_create_no_lock(name);
	}

	fibril_mutex_lock(&namespace->guard);
	namespace->writers_count++;
	fibril_mutex_unlock(&namespace->guard);

	fibril_mutex_unlock(&namespace_list_guard);

	return namespace;
}

void namespace_writer_detach(logging_namespace_t *namespace)
{
	fibril_mutex_lock(&namespace->guard);
	assert(namespace->writers_count > 0);
	namespace->writers_count--;
	fibril_mutex_unlock(&namespace->guard);

	namespace_destroy_careful(namespace);
}

int namespace_change_level(logging_namespace_t *namespace, log_level_t level)
{
	if (level >= LVL_LIMIT)
		return ERANGE;

	fibril_mutex_lock(&namespace->guard);
	namespace->level = level;
	fibril_condvar_broadcast(&namespace->level_changed_cv);
	fibril_mutex_unlock(&namespace->guard);

	return EOK;
}


bool namespace_has_reader(logging_namespace_t *namespace, log_level_t level)
{
	return level <= namespace_get_actual_log_level(namespace);
}

void namespace_wait_for_reader_change(logging_namespace_t *namespace, bool *has_reader_now)
{
	fibril_mutex_lock(&namespace->guard);
	log_level_t previous_level = namespace->level;
	while (previous_level == namespace->level) {
		fibril_condvar_wait(&namespace->level_changed_cv, &namespace->guard);
	}
	*has_reader_now = true;
	fibril_mutex_unlock(&namespace->guard);
}


void namespace_add_message(logging_namespace_t *namespace, const char *message, log_level_t level)
{
	if (level <= namespace_get_actual_log_level(namespace)) {
		const char *level_name = log_level_str(level);
		printf("[%s %s]: %s\n", namespace->name, level_name, message);
		fprintf(namespace->logfile, "%s: %s\n", level_name, message);
		fflush(namespace->logfile);
		fflush(stdout);
	}
}


/**
 * @}
 */
