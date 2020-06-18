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
 * @brief Top utility.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <io/console.h>
#include <io/style.h>
#include <vfs/vfs.h>
#include <stdarg.h>
#include <stats.h>
#include <inttypes.h>
#include <macros.h>
#include <str.h>
#include "screen.h"
#include "top.h"

#define USEC_COUNT  1000000

static suseconds_t timeleft = 0;

console_ctrl_t *console;

static sysarg_t warning_col = 0;
static sysarg_t warning_row = 0;
static suseconds_t warning_timeleft = 0;
static char *warning_text = NULL;

static void screen_style_normal(void)
{
	console_flush(console);
	console_set_style(console, STYLE_NORMAL);
}

static void screen_style_inverted(void)
{
	console_flush(console);
	console_set_style(console, STYLE_INVERTED);
}

static void screen_style_emphasis(void)
{
	console_flush(console);
	console_set_style(console, STYLE_EMPHASIS);
}

static void screen_moveto(sysarg_t col, sysarg_t row)
{
	console_flush(console);
	console_set_pos(console, col, row);
}

static void screen_get_pos(sysarg_t *col, sysarg_t *row)
{
	console_flush(console);
	console_get_pos(console, col, row);
}

static void screen_get_size(sysarg_t *col, sysarg_t *row)
{
	console_flush(console);
	console_get_size(console, col, row);
}

static void screen_restart(bool clear)
{
	screen_style_normal();

	if (clear) {
		console_flush(console);
		console_clear(console);
	}

	screen_moveto(0, 0);
}

static void screen_newline(void)
{
	sysarg_t cols;
	sysarg_t rows;
	screen_get_size(&cols, &rows);

	sysarg_t c;
	sysarg_t r;
	screen_get_pos(&c, &r);

	sysarg_t i;
	for (i = c + 1; i < cols; i++)
		fputs(" ", stdout);

	if (r + 1 < rows)
		puts("");
}

void screen_init(void)
{
	console = console_init(stdin, stdout);

	console_flush(console);
	console_cursor_visibility(console, false);

	screen_restart(true);
}

void screen_done(void)
{
	free(warning_text);
	warning_text = NULL;

	screen_restart(true);

	console_flush(console);
	console_cursor_visibility(console, true);
}

static void print_percent(fixed_float ffloat, unsigned int precision)
{
	printf("%3" PRIu64 ".", ffloat.upper / ffloat.lower);

	unsigned int i;
	uint64_t rest = (ffloat.upper % ffloat.lower) * 10;
	for (i = 0; i < precision; i++) {
		printf("%" PRIu64, rest / ffloat.lower);
		rest = (rest % ffloat.lower) * 10;
	}

	printf("%%");
}

static void print_string(const char *str)
{
	sysarg_t cols;
	sysarg_t rows;
	screen_get_size(&cols, &rows);

	sysarg_t c;
	sysarg_t r;
	screen_get_pos(&c, &r);

	if (c < cols) {
		int pos = cols - c - 1;
		printf("%.*s", pos, str);
	}
}

static inline void print_global_head(data_t *data)
{
	printf("top - %02lu:%02lu:%02lu up "
	    "%" PRIun " days, %02" PRIun ":%02" PRIun ":%02" PRIun ", "
	    "load average:",
	    data->hours, data->minutes, data->seconds,
	    data->udays, data->uhours, data->uminutes, data->useconds);

	size_t i;
	for (i = 0; i < data->load_count; i++) {
		fputs(" ", stdout);
		stats_print_load_fragment(data->load[i], 2);
	}

	screen_newline();
}

static inline void print_task_summary(data_t *data)
{
	printf("tasks: %zu total", data->tasks_count);
	screen_newline();
}

static inline void print_thread_summary(data_t *data)
{
	size_t total = 0;
	size_t running = 0;
	size_t ready = 0;
	size_t sleeping = 0;
	size_t lingering = 0;
	size_t other = 0;
	size_t invalid = 0;

	size_t i;
	for (i = 0; i < data->threads_count; i++) {
		total++;

		switch (data->threads[i].state) {
		case Running:
			running++;
			break;
		case Ready:
			ready++;
			break;
		case Sleeping:
			sleeping++;
			break;
		case Lingering:
			lingering++;
			break;
		case Entering:
		case Exiting:
			other++;
			break;
		default:
			invalid++;
		}
	}

	printf("threads: %zu total, %zu running, %zu ready, "
	    "%zu sleeping, %zu lingering, %zu other, %zu invalid",
	    total, running, ready, sleeping, lingering, other, invalid);
	screen_newline();
}

static inline void print_cpu_info(data_t *data)
{
	size_t i;
	for (i = 0; i < data->cpus_count; i++) {
		if (data->cpus[i].active) {
			uint64_t busy;
			uint64_t idle;
			char busy_suffix;
			char idle_suffix;

			order_suffix(data->cpus[i].busy_cycles, &busy, &busy_suffix);
			order_suffix(data->cpus[i].idle_cycles, &idle, &idle_suffix);

			printf("cpu%u (%4" PRIu16 " MHz): busy cycles: "
			    "%" PRIu64 "%c, idle cycles: %" PRIu64 "%c",
			    data->cpus[i].id, data->cpus[i].frequency_mhz,
			    busy, busy_suffix, idle, idle_suffix);
			fputs(", idle: ", stdout);
			print_percent(data->cpus_perc[i].idle, 2);
			fputs(", busy: ", stdout);
			print_percent(data->cpus_perc[i].busy, 2);
		} else
			printf("cpu%u inactive", data->cpus[i].id);

		screen_newline();
	}
}

static inline void print_physmem_info(data_t *data)
{
	uint64_t total;
	uint64_t unavail;
	uint64_t used;
	uint64_t free;
	const char *total_suffix;
	const char *unavail_suffix;
	const char *used_suffix;
	const char *free_suffix;

	bin_order_suffix(data->physmem->total, &total, &total_suffix, false);
	bin_order_suffix(data->physmem->unavail, &unavail, &unavail_suffix, false);
	bin_order_suffix(data->physmem->used, &used, &used_suffix, false);
	bin_order_suffix(data->physmem->free, &free, &free_suffix, false);

	printf("memory: %" PRIu64 "%s total, %" PRIu64 "%s unavail, %"
	    PRIu64 "%s used, %" PRIu64 "%s free", total, total_suffix,
	    unavail, unavail_suffix, used, used_suffix, free, free_suffix);
	screen_newline();
}

static inline void print_help_head(void)
{
	screen_style_inverted();
	printf("Help");
	screen_newline();
	screen_style_normal();
}

static inline void print_help(void)
{
	sysarg_t cols;
	sysarg_t rows;
	screen_get_size(&cols, &rows);

	screen_newline();

	printf("Operation modes:");
	screen_newline();

	printf(" t .. tasks statistics");
	screen_newline();

	printf(" i .. IPC statistics");
	screen_newline();

	printf(" e .. exceptions statistics");
	screen_newline();

	printf("      a .. toggle display of all/hot exceptions");
	screen_newline();

	printf(" h .. toggle this help screen");
	screen_newline();

	screen_newline();

	printf("Other keys:");
	screen_newline();

	printf(" s .. choose column to sort by");
	screen_newline();

	printf(" r .. toggle reversed sorting");
	screen_newline();

	printf(" q .. quit");
	screen_newline();

	sysarg_t col;
	sysarg_t row;
	screen_get_pos(&col, &row);

	while (row < rows) {
		screen_newline();
		row++;
	}
}

static inline void print_table_head(const table_t *table)
{
	sysarg_t cols;
	sysarg_t rows;
	screen_get_size(&cols, &rows);

	screen_style_inverted();
	for (size_t i = 0; i < table->num_columns; i++) {
		const char *name = table->columns[i].name;
		int width = table->columns[i].width;
		if (i != 0) {
			fputs(" ", stdout);
		}
		if (width == 0) {
			sysarg_t col;
			sysarg_t row;
			screen_get_pos(&col, &row);
			width = cols - col - 1;
		}
		printf("[%-*.*s]", width - 2, width - 2, name);
	}
	screen_newline();
	screen_style_normal();
}

static inline void print_table(const table_t *table)
{
	sysarg_t cols;
	sysarg_t rows;
	screen_get_size(&cols, &rows);

	sysarg_t col;
	sysarg_t row;
	screen_get_pos(&col, &row);

	size_t i;
	for (i = 0; (i < table->num_fields) && (row < rows); i++) {
		size_t column_index = i % table->num_columns;
		int width = table->columns[column_index].width;
		field_t *field = &table->fields[i];
		uint64_t val;
		const char *psuffix;
		char suffix;

		if (column_index != 0) {
			fputs(" ", stdout);
		}

		if (width == 0) {
			screen_get_pos(&col, &row);
			width = cols - col - 1;
		}

		switch (field->type) {
		case FIELD_EMPTY:
			printf("%*s", width, "");
			break;
		case FIELD_UINT:
			printf("%*" PRIu64, width, field->uint);
			break;
		case FIELD_UINT_SUFFIX_BIN:
			val = field->uint;
			width -= 3;
			bin_order_suffix(val, &val, &psuffix, true);
			printf("%*" PRIu64 "%s", width, val, psuffix);
			break;
		case FIELD_UINT_SUFFIX_DEC:
			val = field->uint;
			width -= 1;
			order_suffix(val, &val, &suffix);
			printf("%*" PRIu64 "%c", width, val, suffix);
			break;
		case FIELD_PERCENT:
			width -= 5; /* nnn.% */
			if (width > 2) {
				printf("%*s", width - 2, "");
				width = 2;
			}
			print_percent(field->fixed, width);
			break;
		case FIELD_STRING:
			printf("%-*.*s", width, width, field->string);
			break;
		}

		if (column_index == table->num_columns - 1) {
			screen_newline();
			row++;
		}
	}

	while (row < rows) {
		screen_newline();
		row++;
	}
}

static inline void print_sort(table_t *table)
{
	sysarg_t cols;
	sysarg_t rows;
	screen_get_size(&cols, &rows);

	sysarg_t col;
	sysarg_t row;
	screen_get_pos(&col, &row);

	size_t num = min(table->num_columns, rows - row);
	for (size_t i = 0; i < num; i++) {
		printf("%c - %s", table->columns[i].key, table->columns[i].name);
		screen_newline();
		row++;
	}

	while (row < rows) {
		screen_newline();
		row++;
	}
}

static inline void print_warning(void)
{
	screen_get_pos(&warning_col, &warning_row);
	if (warning_timeleft > 0) {
		screen_style_emphasis();
		print_string(warning_text);
		screen_style_normal();
	} else {
		free(warning_text);
		warning_text = NULL;
	}
	screen_newline();
}

void print_data(data_t *data)
{
	screen_restart(false);
	print_global_head(data);
	print_task_summary(data);
	print_thread_summary(data);
	print_cpu_info(data);
	print_physmem_info(data);
	print_warning();

	switch (screen_mode) {
	case SCREEN_TABLE:
		print_table_head(&data->table);
		print_table(&data->table);
		break;
	case SCREEN_SORT:
		print_sort(&data->table);
		break;
	case SCREEN_HELP:
		print_help_head();
		print_help();
	}

	console_flush(console);
}

void show_warning(const char *fmt, ...)
{
	sysarg_t cols;
	sysarg_t rows;
	screen_get_size(&cols, &rows);

	size_t warning_text_size = 1 + cols * sizeof(*warning_text);
	free(warning_text);
	warning_text = malloc(warning_text_size);
	if (!warning_text)
		return;

	va_list args;
	va_start(args, fmt);
	vsnprintf(warning_text, warning_text_size, fmt, args);
	va_end(args);

	warning_timeleft = 2 * USEC_COUNT;

	screen_moveto(warning_col, warning_row);
	print_warning();
	console_flush(console);
}

/** Get char with timeout
 *
 */
int tgetchar(unsigned int sec)
{
	/*
	 * Reset timeleft whenever it is not positive.
	 */

	if (timeleft <= 0)
		timeleft = sec * USEC_COUNT;

	/*
	 * Wait to see if there is any input. If so, take it and
	 * update timeleft so that the next call to tgetchar()
	 * will not wait as long. If there is no input,
	 * make timeleft zero and return -1.
	 */

	char32_t c = 0;

	while (c == 0) {
		cons_event_t event;

		warning_timeleft -= timeleft;
		if (!console_get_event_timeout(console, &event, &timeleft)) {
			timeleft = 0;
			return -1;
		}
		warning_timeleft += timeleft;

		if (event.type == CEV_KEY && event.ev.key.type == KEY_PRESS)
			c = event.ev.key.c;
	}

	return (int) c;
}

/** @}
 */
