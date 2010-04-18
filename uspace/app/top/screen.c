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
#include <vfs/vfs.h>
#include <stdarg.h>
#include <stats.h>
#include <inttypes.h>
#include "screen.h"
#include "top.h"

#define WHITE  0xf0f0f0
#define BLACK  0x000000

static int rows;
static int colls;
static int up_rows;

static void print_float(fixed_float ffloat, unsigned int precision)
{
	printf("%2" PRIu64 ".", ffloat.upper / ffloat.lower);
	
	unsigned int i;
	uint64_t rest = (ffloat.upper % ffloat.lower) * 10;
	for (i = 0; i < precision; i++) {
		printf("%" PRIu64, rest / ffloat.lower);
		rest = (rest % ffloat.lower) * 10;
	}
}

static void screen_resume_normal(void)
{
	fflush(stdout);
	console_set_rgb_color(fphone(stdout), 0, WHITE);
}

static void screen_moveto(int r, int c)
{
	fflush(stdout);
	console_goto(fphone(stdout), c, r);
}

static void screen_clear(void)
{
	console_clear(fphone(stdout));
	screen_moveto(0, 0);
	up_rows = 0;
	fflush(stdout);
}

void screen_init(void)
{
	console_cursor_visibility(fphone(stdout), 0);
	screen_resume_normal();
	screen_clear();
	
	console_get_size(fphone(stdout), &colls, &rows);
}

void screen_done(void)
{
	screen_resume_normal();
	screen_clear();
	console_cursor_visibility(fphone(stdout), 1);
}

static inline void print_time(data_t *data)
{
	printf("%02lu:%02lu:%02lu ", data->hours, data->minutes, data->seconds);
}

static inline void print_uptime(data_t *data)
{
	printf("up %u days, %02u:%02u:%02u, ", data->udays, data->uhours,
	    data->uminutes, data->useconds);
}

static inline void print_load(data_t *data)
{
	printf("load avarage: ");
	
	size_t i;
	for (i = 0; i < data->load_count; i++) {
		stats_print_load_fragment(data->load[i], 2);
		printf(" ");
	}
}

static inline void print_task_summary(data_t *data)
{
	printf("tasks: %u total", data->tasks_count);
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
	
	printf("threads: %u total, %u running, %u ready, %u sleeping, %u lingering, "
	    "%u other, %u invalid",
	    total, running, ready, sleeping, lingering, other, invalid);
}

static inline void print_cpu_info(data_t *data)
{
	size_t i;
	for (i = 0; i < data->cpus_count; i++) {
		if (data->cpus[i].active) {
			printf("cpu%u (%4" PRIu16 " MHz): busy ticks: "
			    "%" PRIu64 ", idle ticks: %" PRIu64,
			    data->cpus[i].id, data->cpus[i].frequency_mhz,
			    data->cpus[i].busy_ticks, data->cpus[i].idle_ticks);
			printf(", idle: ");
			print_float(data->cpus_perc[i].idle, 2);
			printf("%%, busy: ");
			print_float(data->cpus_perc[i].busy, 2);
			printf("%%\n");
		} else
			printf("cpu%u inactive\n", data->cpus[i].id);
		
		up_rows++;
	}
}

static inline void print_physmem_info(data_t *data)
{
	uint64_t total;
	uint64_t unavail;
	uint64_t used;
	uint64_t free;
	char total_suffix;
	char unavail_suffix;
	char used_suffix;
	char free_suffix;
	
	order_suffix(data->physmem->total, &total, &total_suffix);
	order_suffix(data->physmem->unavail, &unavail, &unavail_suffix);
	order_suffix(data->physmem->used, &used, &used_suffix);
	order_suffix(data->physmem->free, &free, &free_suffix);
	
	printf("memory: %" PRIu64 "%c total, %" PRIu64 "%c unavail, %"
	    PRIu64 "%c used, %" PRIu64 "%c free", total, total_suffix,
	    unavail, unavail_suffix, used, used_suffix, free, free_suffix);
}

static inline void print_tasks(data_t *data, int row)
{
	size_t i;
	for (i = 0; i < data->tasks_count; i++, row++) {
		if (row > rows)
			break;
		
		uint64_t virtmem;
		char virtmem_suffix;
		order_suffix(data->tasks[i].virtmem, &virtmem, &virtmem_suffix);
		
		printf("%8" PRIu64 " %8u %8" PRIu64 "%c ", data->tasks[i].task_id,
		    data->tasks[i].threads, virtmem, virtmem_suffix);
		printf("   ");
		print_float(data->tasks_perc[i].virtmem, 2);
		printf("%%   ");
		print_float(data->tasks_perc[i].ucycles, 2);
		printf("%%   ");
		print_float(data->tasks_perc[i].kcycles, 2);
		printf("%% %s\n", data->tasks[i].name);
	}
}

static inline void print_task_head(void)
{
	fflush(stdout);
	console_set_rgb_color(fphone(stdout), WHITE, BLACK);
	
	printf("      ID  Threads      Mem      %%Mem %%uCycles %%kCycles  Name");
	
	int i;
	for (i = 61; i < colls; ++i)
		printf(" ");
	
	fflush(stdout);
	console_set_rgb_color(fphone(stdout), BLACK, WHITE);
}

static inline void print_ipc_head(void)
{
	fflush(stdout);
	console_set_rgb_color(fphone(stdout), WHITE, BLACK);
	
	printf("      ID Calls sent Calls recv Answs sent Answs recv  IRQn recv       Forw Name");
	
	int i;
	for (i = 80; i < colls; ++i)
		printf(" ");
	
	fflush(stdout);
	console_set_rgb_color(fphone(stdout), BLACK, WHITE);
}

static inline void print_ipc(data_t *data, int row)
{
	size_t i;
	for (i = 0; i < data->tasks_count; i++, row++) {
		if (row > rows)
			break;
		
		printf("%8" PRIu64 " %10" PRIu64 " %10" PRIu64 " %10" PRIu64
		     " %10" PRIu64 " %10" PRIu64 " %10" PRIu64 " %s\n",
		     data->tasks[i].task_id, data->tasks[i].ipc_info.call_sent,
		     data->tasks[i].ipc_info.call_recieved,
		     data->tasks[i].ipc_info.answer_sent,
		     data->tasks[i].ipc_info.answer_recieved,
		     data->tasks[i].ipc_info.irq_notif_recieved,
		     data->tasks[i].ipc_info.forwarded, data->tasks[i].name);
	}
}

void print_data(data_t *data)
{
	screen_clear();
	fflush(stdout);
	
	printf("top - ");
	print_time(data);
	print_uptime(data);
	print_load(data);
	
	printf("\n");
	up_rows++;
	
	print_task_summary(data);
	
	printf("\n");
	up_rows++;
	
	print_thread_summary(data);
	
	printf("\n");
	up_rows++;
	
	print_cpu_info(data);
	print_physmem_info(data);
	
	printf("\n");
	up_rows++;
	
	/* Empty row for warnings */
	printf("\n");
	
	if (operation_type == OP_IPC) {
		print_ipc_head();
		printf("\n");
		print_ipc(data, up_rows);
	} else {
		print_task_head();
		printf("\n");
		print_tasks(data, up_rows);
	}
	
	fflush(stdout);
}

void print_warning(const char *fmt, ...)
{
	screen_moveto(up_rows, 0);
	
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	
	fflush(stdout);
}

/** @}
 */
