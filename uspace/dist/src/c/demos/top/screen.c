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
#include <io/console.h>
#include <io/style.h>
#include <vfs/vfs.h>
#include <stdarg.h>
#include <stats.h>
#include <inttypes.h>
#include "screen.h"
#include "top.h"

#define USEC_COUNT  1000000

static sysarg_t warn_col = 0;
static sysarg_t warn_row = 0;
static suseconds_t timeleft = 0;

console_ctrl_t *console;

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
		puts(" ");
	
	if (r + 1 < rows)
		puts("\n");
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
		puts(" ");
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
			puts(", idle: ");
			print_percent(data->cpus_perc[i].idle, 2);
			puts(", busy: ");
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

static inline void print_tasks_head(void)
{
	screen_style_inverted();
	printf("[taskid] [thrds] [resident] [%%resi] [virtual] [%%virt]"
	    " [%%user] [%%kern] [name");
	screen_newline();
	screen_style_normal();
}

static inline void print_tasks(data_t *data)
{
	sysarg_t cols;
	sysarg_t rows;
	screen_get_size(&cols, &rows);
	
	sysarg_t col;
	sysarg_t row;
	screen_get_pos(&col, &row);
	
	size_t i;
	for (i = 0; (i < data->tasks_count) && (row < rows); i++, row++) {
		stats_task_t *task = data->tasks + data->tasks_map[i];
		perc_task_t *perc = data->tasks_perc + data->tasks_map[i];
		
		uint64_t resmem;
		const char *resmem_suffix;
		bin_order_suffix(task->resmem, &resmem, &resmem_suffix, true);
		
		uint64_t virtmem;
		const char *virtmem_suffix;
		bin_order_suffix(task->virtmem, &virtmem, &virtmem_suffix, true);
		
		printf("%-8" PRIu64 " %7zu %7" PRIu64 "%s ",
		    task->task_id, task->threads, resmem, resmem_suffix);
		print_percent(perc->resmem, 2);
		printf(" %6" PRIu64 "%s ", virtmem, virtmem_suffix);
		print_percent(perc->virtmem, 2);
		puts(" ");
		print_percent(perc->ucycles, 2);
		puts(" ");
		print_percent(perc->kcycles, 2);
		puts(" ");
		print_string(task->name);
		
		screen_newline();
	}
	
	while (row < rows) {
		screen_newline();
		row++;
	}
}

static inline void print_ipc_head(void)
{
	screen_style_inverted();
	printf("[taskid] [cls snt] [cls rcv] [ans snt]"
	    " [ans rcv] [irq rcv] [forward] [name");
	screen_newline();
	screen_style_normal();
}

static inline void print_ipc(data_t *data)
{
	sysarg_t cols;
	sysarg_t rows;
	screen_get_size(&cols, &rows);
	
	sysarg_t col;
	sysarg_t row;
	screen_get_pos(&col, &row);
	
	size_t i;
	for (i = 0; (i < data->tasks_count) && (row < rows); i++, row++) {
		uint64_t call_sent;
		uint64_t call_received;
		uint64_t answer_sent;
		uint64_t answer_received;
		uint64_t irq_notif_received;
		uint64_t forwarded;
		
		char call_sent_suffix;
		char call_received_suffix;
		char answer_sent_suffix;
		char answer_received_suffix;
		char irq_notif_received_suffix;
		char forwarded_suffix;
		
		order_suffix(data->tasks[i].ipc_info.call_sent, &call_sent,
		    &call_sent_suffix);
		order_suffix(data->tasks[i].ipc_info.call_received,
		    &call_received, &call_received_suffix);
		order_suffix(data->tasks[i].ipc_info.answer_sent,
		    &answer_sent, &answer_sent_suffix);
		order_suffix(data->tasks[i].ipc_info.answer_received,
		    &answer_received, &answer_received_suffix);
		order_suffix(data->tasks[i].ipc_info.irq_notif_received,
		    &irq_notif_received, &irq_notif_received_suffix);
		order_suffix(data->tasks[i].ipc_info.forwarded, &forwarded,
		    &forwarded_suffix);
		
		printf("%-8" PRIu64 " %8" PRIu64 "%c %8" PRIu64 "%c"
		     " %8" PRIu64 "%c %8" PRIu64 "%c %8" PRIu64 "%c"
		     " %8" PRIu64 "%c ", data->tasks[i].task_id,
		     call_sent, call_sent_suffix,
		     call_received, call_received_suffix,
		     answer_sent, answer_sent_suffix,
		     answer_received, answer_received_suffix,
		     irq_notif_received, irq_notif_received_suffix,
		     forwarded, forwarded_suffix);
		print_string(data->tasks[i].name);
		
		screen_newline();
	}
	
	while (row < rows) {
		screen_newline();
		row++;
	}
}

static inline void print_excs_head(void)
{
	screen_style_inverted();
	printf("[exc   ] [count   ] [%%count] [cycles  ] [%%cycles] [description");
	screen_newline();
	screen_style_normal();
}

static inline void print_excs(data_t *data)
{
	sysarg_t cols;
	sysarg_t rows;
	screen_get_size(&cols, &rows);
	
	sysarg_t col;
	sysarg_t row;
	screen_get_pos(&col, &row);
	
	size_t i;
	for (i = 0; (i < data->exceptions_count) && (row < rows); i++) {
		/* Filter-out cold exceptions if not instructed otherwise */
		if ((!excs_all) && (!data->exceptions[i].hot))
			continue;
		
		uint64_t count;
		uint64_t cycles;
		
		char count_suffix;
		char cycles_suffix;
		
		order_suffix(data->exceptions[i].count, &count, &count_suffix);
		order_suffix(data->exceptions[i].cycles, &cycles, &cycles_suffix);
		
		printf("%-8u %9" PRIu64 "%c  ",
		     data->exceptions[i].id, count, count_suffix);
		print_percent(data->exceptions_perc[i].count, 2);
		printf(" %9" PRIu64 "%c   ", cycles, cycles_suffix);
		print_percent(data->exceptions_perc[i].cycles, 2);
		puts(" ");
		print_string(data->exceptions[i].desc);
		
		screen_newline();
		row++;
	}
	
	while (row < rows) {
		screen_newline();
		row++;
	}
}

static void print_help(void)
{
	sysarg_t cols;
	sysarg_t rows;
	screen_get_size(&cols, &rows);
	
	sysarg_t col;
	sysarg_t row;
	screen_get_pos(&col, &row);
	
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
	
	row += 6;
	
	while (row < rows) {
		screen_newline();
		row++;
	}
}

void print_data(data_t *data)
{
	screen_restart(false);
	print_global_head(data);
	print_task_summary(data);
	print_thread_summary(data);
	print_cpu_info(data);
	print_physmem_info(data);
	
	/* Empty row for warnings */
	screen_get_pos(&warn_col, &warn_row);
	screen_newline();
	
	switch (op_mode) {
	case OP_TASKS:
		print_tasks_head();
		print_tasks(data);
		break;
	case OP_IPC:
		print_ipc_head();
		print_ipc(data);
		break;
	case OP_EXCS:
		print_excs_head();
		print_excs(data);
		break;
	case OP_HELP:
		print_tasks_head();
		print_help();
	}
	
	console_flush(console);
}

void print_warning(const char *fmt, ...)
{
	screen_moveto(warn_col, warn_row);
	
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	
	screen_newline();
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
	
	wchar_t c = 0;
	
	while (c == 0) {
		kbd_event_t event;
		
		if (!console_get_kbd_event_timeout(console, &event, &timeleft)) {
			timeleft = 0;
			return -1;
		}
		
		if (event.type == KEY_PRESS)
			c = event.c;
	}
	
	return (int) c;
}

/** @}
 */
