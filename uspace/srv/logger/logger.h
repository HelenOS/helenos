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
/** @file Common logger service definitions.
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <adt/list.h>
#include <adt/prodcons.h>
#include <io/log.h>
#include <async.h>
#include <stdbool.h>
#include <fibril_synch.h>
#include <stdio.h>

#define NAME "logger"
#define LOG_LEVEL_USE_DEFAULT (LVL_LIMIT + 1)

#ifdef LOGGER_LOG
#define logger_log(fmt, ...) printf(NAME ": " fmt, ##__VA_ARGS__)
#else
#define logger_log(fmt, ...) (void)0
#endif

typedef struct logger_log logger_log_t;

typedef struct {
	fibril_mutex_t guard;
	char *filename;
	FILE *logfile;
} logger_dest_t;

struct logger_log {
	link_t link;

	size_t ref_counter;

	fibril_mutex_t guard;

	char *name;
	char *full_name;
	logger_log_t *parent;
	log_level_t logged_level;
	logger_dest_t *dest;
};

#define MAX_REFERENCED_LOGS_PER_CLIENT 100

typedef struct {
	size_t logs_count;
	logger_log_t *logs[MAX_REFERENCED_LOGS_PER_CLIENT];
} logger_registered_logs_t;

logger_log_t *find_log_by_name_and_lock(const char *name);
logger_log_t *find_or_create_log_and_lock(const char *, sysarg_t);
logger_log_t *find_log_by_id_and_lock(sysarg_t);
bool shall_log_message(logger_log_t *, log_level_t);
void log_unlock(logger_log_t *);
void write_to_log(logger_log_t *, log_level_t, const char *);
void log_release(logger_log_t *);

void registered_logs_init(logger_registered_logs_t *);
bool register_log(logger_registered_logs_t *, logger_log_t *);
void unregister_logs(logger_registered_logs_t *);

log_level_t get_default_logging_level(void);
errno_t set_default_logging_level(log_level_t);

void logger_connection_handler_control(ipc_call_t *);
void logger_connection_handler_writer(ipc_call_t *);

void parse_initial_settings(void);
void parse_level_settings(char *);

#endif

/** @}
 */
