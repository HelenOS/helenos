/*
 * Copyright (c) 2010 Jiri Svoboda
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

/** @addtogroup taskdump
 * @{
 */
/** @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ipc/ipc.h>
#include <errno.h>
#include <udebug.h>
#include <task.h>
#include <kernel/mm/as.h>
#include <libarch/istate.h>
#include <macros.h>
#include <assert.h>
#include <bool.h>

#include <symtab.h>
#include <stacktrace.h>

#define LINE_BYTES 16

#define DBUF_SIZE 4096
static uint8_t data_buf[DBUF_SIZE];

static int phoneid;
static task_id_t task_id;
static bool dump_memory;
static char *app_name;
static symtab_t *app_symtab;

static int connect_task(task_id_t task_id);
static int parse_args(int argc, char *argv[]);
static void print_syntax(void);
static int threads_dump(void);
static int thread_dump(uintptr_t thash);
static int areas_dump(void);
static int area_dump(as_area_info_t *area);
static void hex_dump(uintptr_t addr, void *buffer, size_t size);
static int td_read_uintptr(void *arg, uintptr_t addr, uintptr_t *value);

static void autoload_syms(void);
static char *get_app_task_name(void);
static char *fmt_sym_address(uintptr_t addr);

int main(int argc, char *argv[])
{
	int rc;

	/*
	 * FIXME: The stdio module cannot currently detect whether we are
	 * writing to a console or file. This workaround make file output
	 * faster.
	 */
	setvbuf(stdout, NULL, _IOFBF, 32768);

	printf("Task Dump Utility\n");
	dump_memory = false;

	if (parse_args(argc, argv) < 0)
		return 1;

	rc = connect_task(task_id);
	if (rc < 0) {
		printf("Failed connecting to task %lld.\n", task_id);
		return 1;
	}

	app_name = get_app_task_name();
	app_symtab = NULL;

	printf("Dumping task '%s' (task ID %lld).\n", app_name, task_id);
	autoload_syms();
	putchar('\n');

	rc = threads_dump();
	if (rc < 0)
		printf("Failed dumping threads.\n");

	rc = areas_dump();
	if (rc < 0)
		printf("Failed dumping address space areas.\n");

	udebug_end(phoneid);
	ipc_hangup(phoneid);

	return 0;
}

static int connect_task(task_id_t task_id)
{
	int rc;

	rc = ipc_connect_kbox(task_id);

	if (rc == ENOTSUP) {
		printf("You do not have userspace debugging support "
		    "compiled in the kernel.\n");
		printf("Compile kernel with 'Support for userspace debuggers' "
		    "(CONFIG_UDEBUG) enabled.\n");
		return rc;
	}

	if (rc < 0) {
		printf("Error connecting\n");
		printf("ipc_connect_task(%lld) -> %d ", task_id, rc);
		return rc;
	}

	phoneid = rc;

	rc = udebug_begin(phoneid);
	if (rc < 0) {
		printf("udebug_begin() -> %d\n", rc);
		return rc;
	}

	return 0;
}

static int parse_args(int argc, char *argv[])
{
	char *arg;
	char *err_p;

	task_id = 0;

	--argc; ++argv;

	while (argc > 0) {
		arg = *argv;
		if (arg[0] == '-') {
			if (arg[1] == 't' && arg[2] == '\0') {
				/* Task ID */
				--argc; ++argv;
				task_id = strtol(*argv, &err_p, 10);
				if (*err_p) {
					printf("Task ID syntax error\n");
					print_syntax();
					return -1;
				}
			} else if (arg[1] == 'm' && arg[2] == '\0') {
				dump_memory = true;
			} else {
				printf("Uknown option '%s'\n", arg[0]);
				print_syntax();
				return -1;
			}
		} else {
			break;
		}

		--argc; ++argv;
	}

	if (task_id == 0) {
		printf("Missing task ID argument\n");
		print_syntax();
		return -1;
	}

	if (argc != 0) {
		printf("Extra arguments\n");
		print_syntax();
		return -1;
	}

	return 0;
}

static void print_syntax(void)
{
	printf("Syntax: taskdump [-m] -t <task_id>\n");
	printf("\t-m\tDump memory area contents.\n");
	printf("\t-t <task_id>\tWhich task to dump.\n");
}

static int threads_dump(void)
{
	uintptr_t *thash_buf;
	uintptr_t dummy_buf;
	size_t buf_size, n_threads;

	size_t copied;
	size_t needed;
	size_t i;
	int rc;

	/* TODO: See why NULL does not work. */
	rc = udebug_thread_read(phoneid, &dummy_buf, 0, &copied, &needed);
	if (rc < 0) {
		printf("udebug_thread_read() -> %d\n", rc);
		return rc;
	}

	if (needed == 0) {
		printf("No threads.\n\n");
		return 0;
	}

	buf_size = needed;
	thash_buf = malloc(buf_size);

	rc = udebug_thread_read(phoneid, thash_buf, buf_size, &copied, &needed);
	if (rc < 0) {
		printf("udebug_thread_read() -> %d\n", rc);
		return rc;
	}

	assert(copied == buf_size);
	assert(needed == buf_size);

	n_threads = copied / sizeof(uintptr_t);

	printf("Threads:\n");
	for (i = 0; i < n_threads; i++) {
		printf(" [%d] hash: 0x%lx\n", 1+i, thash_buf[i]);

		thread_dump(thash_buf[i]);
	}
	putchar('\n');

	free(thash_buf);

	return 0;
}

static int areas_dump(void)
{
	as_area_info_t *ainfo_buf;
	as_area_info_t dummy_buf;
	size_t buf_size, n_areas;

	size_t copied;
	size_t needed;
	size_t i;
	int rc;

	rc = udebug_areas_read(phoneid, &dummy_buf, 0, &copied, &needed);
	if (rc < 0) {
		printf("udebug_areas_read() -> %d\n", rc);
		return rc;
	}

	buf_size = needed;
	ainfo_buf = malloc(buf_size);

	rc = udebug_areas_read(phoneid, ainfo_buf, buf_size, &copied, &needed);
	if (rc < 0) {
		printf("udebug_areas_read() -> %d\n", rc);
		return rc;
	}

	assert(copied == buf_size);
	assert(needed == buf_size);

	n_areas = copied / sizeof(as_area_info_t);

	printf("Address space areas:\n");
	for (i = 0; i < n_areas; i++) {
		printf(" [%d] flags: %c%c%c%c base: 0x%lx size: 0x%lx\n", 1+i,
		    (ainfo_buf[i].flags & AS_AREA_READ) ? 'R' : '-',
		    (ainfo_buf[i].flags & AS_AREA_WRITE) ? 'W' : '-',
		    (ainfo_buf[i].flags & AS_AREA_EXEC) ? 'X' : '-',
		    (ainfo_buf[i].flags & AS_AREA_CACHEABLE) ? 'C' : '-',
		    ainfo_buf[i].start_addr, ainfo_buf[i].size);

		if (dump_memory) {
			putchar('\n');
			area_dump(&ainfo_buf[i]);
			putchar('\n');
		}
	}

	putchar('\n');

	free(ainfo_buf);

	return 0;
}

static int thread_dump(uintptr_t thash)
{
	istate_t istate;
	uintptr_t pc, fp, nfp;
	stacktrace_t st;
	char *sym_pc;
	int rc;

	rc = udebug_regs_read(phoneid, thash, &istate);
	if (rc < 0) {
		printf("Failed reading registers (%d).\n", rc);
		return EIO;
	}

	pc = istate_get_pc(&istate);
	fp = istate_get_fp(&istate);

	sym_pc = fmt_sym_address(pc);
	printf("Thread 0x%lx crashed at %s. FP = 0x%lx\n", thash, sym_pc, fp);
	free(sym_pc);

	st.op_arg = NULL;
	st.read_uintptr = td_read_uintptr;

	while (stacktrace_fp_valid(&st, fp)) {
		sym_pc = fmt_sym_address(pc);
		printf("  %p: %s\n", fp, sym_pc);
		free(sym_pc);

		rc = stacktrace_ra_get(&st, fp, &pc);
		if (rc != EOK)
			return rc;

		rc = stacktrace_fp_prev(&st, fp, &nfp);
		if (rc != EOK)
			return rc;

		fp = nfp;
	}

	return EOK;
}

static int area_dump(as_area_info_t *area)
{
	size_t to_copy;
	size_t total;
	uintptr_t addr;
	int rc;

	addr = area->start_addr;
	total = 0;

	while (total < area->size) {
		to_copy = min(area->size - total, DBUF_SIZE);
		rc = udebug_mem_read(phoneid, data_buf, addr, to_copy);
		if (rc < 0) {
			printf("udebug_mem_read() failed.\n");
			return rc;
		}

		hex_dump(addr, data_buf, to_copy);

		addr += to_copy;
		total += to_copy;
	}

	return EOK;
}

static void hex_dump(uintptr_t addr, void *buffer, size_t size)
{
	uint8_t *data = (uint8_t *) buffer;
	uint8_t b;
	size_t pos, i;

	assert(addr % LINE_BYTES == 0);
	assert(size % LINE_BYTES == 0);

	pos = 0;

	while (pos < size) {
		printf("%08lx:", addr + pos);
		for (i = 0; i < LINE_BYTES; ++i) {
			if (i % 4 == 0) putchar(' ');
			printf(" %02x", data[pos + i]);
		}
		putchar('\t');

		for (i = 0; i < LINE_BYTES; ++i) {
			b = data[pos + i];
			if (b >= 32 && b < 127) {
				putchar(b);
			} else {
				putchar(' ');
			}
		}
		putchar('\n');
		pos += LINE_BYTES;
	}
}

static int td_read_uintptr(void *arg, uintptr_t addr, uintptr_t *value)
{
	uintptr_t data;
	int rc;

	(void) arg;

	rc = udebug_mem_read(phoneid, &data, addr, sizeof(data));
	if (rc < 0) {
		printf("Warning: udebug_mem_read() failed.\n");
		return rc;
	}

	*value = data;
	return EOK;
}

/** Attempt to find the right executable file and load the symbol table. */
static void autoload_syms(void)
{
	char *file_name;
	int rc;

	assert(app_name != NULL);
	assert(app_symtab == NULL);

	rc = asprintf(&file_name, "/app/%s", app_name);
	if (rc < 0) {
		printf("Memory allocation failure.\n");
		exit(1);
	}

	rc = symtab_load(file_name, &app_symtab);
	if (rc == EOK) {
		printf("Loaded symbol table from %s\n", file_name);
		free(file_name);
		return;
	}

	free(file_name);

	rc = asprintf(&file_name, "/srv/%s", app_name);
	if (rc < 0) {
		printf("Memory allocation failure.\n");
		exit(1);
	}

	rc = symtab_load(file_name, &app_symtab);
	if (rc == EOK) {
		printf("Loaded symbol table from %s\n", file_name);
		free(file_name);
		return;
	}

	free(file_name);
	printf("Failed autoloading symbol table.\n");
}

static char *get_app_task_name(void)
{
	char dummy_buf;
	size_t copied, needed, name_size;
	char *name;
	int rc;

	rc = udebug_name_read(phoneid, &dummy_buf, 0, &copied, &needed);
	if (rc < 0)
		return NULL;

	name_size = needed;
	name = malloc(name_size + 1);
	rc = udebug_name_read(phoneid, name, name_size, &copied, &needed);
	if (rc < 0) {
		free(name);
		return NULL;
	}

	assert(copied == name_size);
	assert(copied == needed);
	name[copied] = '\0';

	return name;
}

/** Format address in symbolic form.
 *
 * Formats address as <symbol_name>+<offset> (<address>), if possible,
 * otherwise as <address>.
 *
 * @param addr	Address to format.
 * @return	Newly allocated string, address in symbolic form.
 */
static char *fmt_sym_address(uintptr_t addr)
{
	char *name;
	size_t offs;
	int rc;
	char *str;

	if (app_symtab != NULL) {
		rc = symtab_addr_to_name(app_symtab, addr, &name, &offs);
	} else {
		rc = ENOTSUP;
	}

	if (rc == EOK) {
		rc = asprintf(&str, "%p (%s+%p)", addr, name, offs);
	} else {
		rc = asprintf(&str, "%p", addr);
	}

	if (rc < 0) {
		printf("Memory allocation error.\n");
		exit(1);
	}

	return str;
}

/** @}
 */
