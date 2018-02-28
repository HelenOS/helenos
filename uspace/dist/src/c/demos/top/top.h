/*
 * Copyright (c) 2010 Stanislav Kozina
 * Copyright (c) 2010 Martin Decky
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

/** @addtogroup top
 * @{
 */

#ifndef TOP_TOP_H_
#define TOP_TOP_H_

#include <task.h>
#include <stats.h>
#include <time.h>

#define FRACTION_TO_FLOAT(float, a, b) \
	do { \
		if ((b) != 0) { \
			(float).upper = (a); \
			(float).lower = (b); \
		} else { \
			(float).upper = 0; \
			(float).lower = 1; \
		} \
	} while (0)

typedef enum {
	SCREEN_TABLE,
	SCREEN_SORT,
	SCREEN_HELP,
} screen_mode_t;

extern screen_mode_t screen_mode;

typedef struct {
	uint64_t upper;
	uint64_t lower;
} fixed_float;

typedef struct {
	fixed_float idle;
	fixed_float busy;
} perc_cpu_t;

typedef struct {
	fixed_float virtmem;
	fixed_float resmem;
	fixed_float ucycles;
	fixed_float kcycles;
} perc_task_t;

typedef struct {
	fixed_float cycles;
	fixed_float count;
} perc_exc_t;

typedef enum {
	FIELD_EMPTY, FIELD_UINT, FIELD_UINT_SUFFIX_BIN, FIELD_UINT_SUFFIX_DEC,
	FIELD_PERCENT, FIELD_STRING
} field_type_t;

typedef struct {
	field_type_t type;
	union {
		fixed_float fixed;
		uint64_t uint;
		const char *string;
	};
} field_t;

typedef struct {
	const char *name;
	char key;
	int width;
} column_t;

typedef struct {
	const char *name;
	size_t num_columns;
	const column_t *columns;
	size_t num_fields;
	field_t *fields;
} table_t;

typedef struct {
	time_t hours;
	time_t minutes;
	time_t seconds;

	sysarg_t udays;
	sysarg_t uhours;
	sysarg_t uminutes;
	sysarg_t useconds;

	size_t load_count;
	load_t *load;

	size_t cpus_count;
	stats_cpu_t *cpus;
	perc_cpu_t *cpus_perc;

	size_t tasks_count;
	stats_task_t *tasks;
	perc_task_t *tasks_perc;

	size_t threads_count;
	stats_thread_t *threads;

	size_t exceptions_count;
	stats_exc_t *exceptions;
	perc_exc_t *exceptions_perc;

	stats_physmem_t *physmem;

	uint64_t *ucycles_diff;
	uint64_t *kcycles_diff;
	uint64_t *ecycles_diff;
	uint64_t *ecount_diff;

	table_t table;
} data_t;

#endif

/**
 * @}
 */
