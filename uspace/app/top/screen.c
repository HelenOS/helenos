/*
 * Copyright (c) 2010 Stanislav Kozina
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
#include <load.h>
#include <kernel/ps/taskinfo.h>
#include <ps.h>
#include "screen.h"
#include "top.h"
#include "func.h"

int rows;
int colls;
int up_rows;

#define WHITE 0xf0f0f0
#define BLACK 0x000000

static void print_float(float f, int precision)
{
	printf("%2u.", (unsigned int) f);
	int i;
	float rest = (f - (int)f) * 10;
	for (i = 0; i < precision; ++i) {
		printf("%d", (unsigned int)rest);
		rest = (rest - (int)rest) * 10;
	}
}

static void resume_normal(void)
{
	fflush(stdout);
	console_set_rgb_color(fphone(stdout), 0, WHITE);
}

void screen_init(void)
{
	console_get_size(fphone(stdout), &colls, &rows);
	up_rows = 0;
	console_cursor_visibility(fphone(stdout), 0);
	resume_normal();
	clear_screen();
}

void clear_screen(void)
{
	console_clear(fphone(stdout));
	moveto(0, 0);
	up_rows = 0;
	fflush(stdout);
}

void moveto(int r, int c)
{
	fflush(stdout);
	console_goto(fphone(stdout), c, r);
}

static inline void print_time(data_t *data)
{
	printf("%02d:%02d:%02d ", data->hours, data->minutes, data->seconds);
}

static inline void print_uptime(data_t *data)
{
	printf("up %4d days, %02d:%02d:%02d, ", data->uptime_d, data->uptime_h,
		data->uptime_m, data->uptime_s);
}

static inline void print_load(data_t *data)
{
	puts("load avarage: ");
	print_load_fragment(data->load[0], 2);
	puts(" ");
	print_load_fragment(data->load[1], 2);
	puts(" ");
	print_load_fragment(data->load[2], 2);
}

static inline void print_taskstat(data_t *data)
{
	puts("Tasks: ");
	printf("%4u total", data->task_count);
}

static inline void print_threadstat(data_t *data)
{
	size_t sleeping = 0;
	size_t running = 0;
	size_t invalid = 0;
	size_t other = 0;
	size_t total = 0;
	size_t i;
	for (i = 0; i < data->thread_count; ++i) {
		++total;
		switch (data->thread_infos[i].state) {
			case Invalid:
			case Lingering:
				++invalid;
				break;
			case Running:
			case Ready:
				++running;
				break;
			case Sleeping:
				++sleeping;
				break;
			case Entering:
			case Exiting:
				++other;
				break;
		}
	}
	printf("Threads: %5u total, %5u running, %5u sleeping, %5u invalid, %5u other",
		total, running, sleeping, invalid, other);
}

static inline void print_cpuinfo(data_t *data)
{
	unsigned int i;
	uspace_cpu_info_t *cpus = data->cpus;
	for (i = 0; i < data->cpu_count; ++i) {
		printf("Cpu%u (%4u Mhz): Busy ticks: %6llu, Idle Ticks: %6llu",
			i, (unsigned int)cpus[i].frequency_mhz, cpus[i].busy_ticks,
			cpus[i].idle_ticks);
		printf(", idle: ");
		print_float(data->cpu_perc[i].idle, 2);
		puts("%, busy: ");
		print_float(data->cpu_perc[i].busy, 2);
		puts("%\n");
		++up_rows;
	}
}

static inline void print_meminfo(data_t *data)
{
	uint64_t newsize;
	char suffix;
	order(data->mem_info.total, &newsize, &suffix);
	printf("Mem: %8llu %c total", newsize, suffix);
	order(data->mem_info.used, &newsize, &suffix);
	printf(", %8llu %c used", newsize, suffix);
	order(data->mem_info.free, &newsize, &suffix);
	printf(", %8llu %c free", newsize, suffix);
}

static inline void print_tasks(data_t *data, int row)
{
	int i;
	for (i = 0; i < (int)data->task_count; ++i) {
		if (row + i > rows)
			return;
		task_info_t *taskinfo = &data->taskinfos[i];
		uint64_t mem;
		char suffix;
		order(taskinfo->virt_mem, &mem, &suffix);
		printf("%8llu %8u %8llu%c ", taskinfo->taskid,
			taskinfo->thread_count, mem, suffix);
		task_perc_t *taskperc = &data->task_perc[i];
		puts("   ");
		print_float(taskperc->mem, 2);
		puts("%   ");
		print_float(taskperc->ucycles, 2);
		puts("%   ");
		print_float(taskperc->kcycles, 2);
		puts("% ");
		printf("%s\n", taskinfo->name);
	}
}

static inline void print_head(void)
{
	fflush(stdout);
	console_set_rgb_color(fphone(stdout), WHITE, BLACK);
	printf("      ID  Threads      Mem      %%Mem %%uCycles %%kCycles Name");
	int i;
	for (i = 60; i < colls; ++i)
		puts(" ");
	fflush(stdout);
	console_set_rgb_color(fphone(stdout), BLACK, WHITE);
}

void print_data(data_t *data)
{
	clear_screen();
	fflush(stdout);
	printf("top - ");
	print_time(data);
	print_uptime(data);
	print_load(data);
	puts("\n");
	++up_rows;
	print_taskstat(data);
	puts("\n");
	++up_rows;
	print_threadstat(data);
	puts("\n");
	++up_rows;
	print_cpuinfo(data);
	print_meminfo(data);
	puts("\n");
	++up_rows;
	puts("\n");
	++up_rows;
	print_head();
	puts("\n");
	print_tasks(data, up_rows);
	fflush(stdout);
}

/** @}
 */
