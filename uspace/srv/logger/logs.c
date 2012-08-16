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

static FIBRIL_MUTEX_INITIALIZE(toplog_list_guard);
static LIST_INITIALIZE(toplog_list);

static logger_toplevel_log_t *find_log_by_name_no_lock(const char *name)
{
	list_foreach(toplog_list, it) {
		logger_toplevel_log_t *log = list_get_instance(it, logger_toplevel_log_t, link);
		if (str_cmp(log->name, name) == 0) {
			return log;
		}
	}

	return NULL;
}

logger_toplevel_log_t *find_or_create_toplevel_log(const char *name)
{
	logger_toplevel_log_t *result = NULL;

	fibril_mutex_lock(&toplog_list_guard);

	result = find_log_by_name_no_lock(name);
	if (result != NULL)
		goto leave;

	result = malloc(sizeof(logger_toplevel_log_t));
	if (result == NULL)
		goto leave;

	char *logfilename;
	asprintf(&logfilename, "/log/%s", name);
	result->logfile = fopen(logfilename, "a");
	result->logged_level = LOG_LEVEL_USE_DEFAULT;
	result->name = str_dup(name);
	result->sublog_count = 1;
	result->sublogs[0].name = "";
	result->sublogs[0].logged_level = LOG_LEVEL_USE_DEFAULT;

	link_initialize(&result->link);

	list_append(&result->link, &toplog_list);

leave:
	fibril_mutex_unlock(&toplog_list_guard);

	return result;
}

logger_toplevel_log_t *find_toplevel_log(sysarg_t id)
{
	list_foreach(toplog_list, it) {
		logger_toplevel_log_t *log = list_get_instance(it, logger_toplevel_log_t, link);
		if ((sysarg_t) log == id)
			return log;
	}

	return NULL;
}

bool shall_log_message(logger_toplevel_log_t *toplog, sysarg_t log, log_level_t level)
{
	if (log >= toplog->sublog_count)
		return false;

	log_level_t logged_level = toplog->sublogs[log].logged_level;
	if (logged_level == LOG_LEVEL_USE_DEFAULT) {
		logged_level = toplog->logged_level;
		if (logged_level == LOG_LEVEL_USE_DEFAULT)
			logged_level = get_default_logging_level();
	}


	return level <= logged_level;
}

int add_sub_log(logger_toplevel_log_t *toplog, const char *name, sysarg_t *id)
{
	if (toplog->sublog_count >= MAX_SUBLOGS)
		return ELIMIT;

	logger_sublog_t *sublog =  &toplog->sublogs[toplog->sublog_count];
	sublog->name = str_dup(name);
	sublog->logged_level = toplog->logged_level;

	*id = toplog->sublog_count;

	toplog->sublog_count++;

	return EOK;
}


/**
 * @}
 */
