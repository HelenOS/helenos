/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup genericconsole
 * @{
 */

/**
 * @file  cmd.c
 * @brief Kernel console command wrappers.
 *
 * This file is meant to contain all wrapper functions for
 * all kconsole commands. The point is in separating
 * kconsole specific wrappers from kconsole-unaware functions
 * from other subsystems.
 */

#include <assert.h>
#include <console/cmd.h>
#include <console/console.h>
#include <console/kconsole.h>
#include <print.h>
#include <log.h>
#include <panic.h>
#include <typedefs.h>
#include <adt/list.h>
#include <arch.h>
#include <config.h>
#include <halt.h>
#include <str.h>
#include <macros.h>
#include <cpu.h>
#include <mm/tlb.h>
#include <mm/km.h>
#include <arch/mm/tlb.h>
#include <mm/frame.h>
#include <main/version.h>
#include <mm/slab.h>
#include <proc/scheduler.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <ipc/ipc.h>
#include <ipc/irq.h>
#include <ipc/event.h>
#include <sysinfo/sysinfo.h>
#include <symtab.h>
#include <synch/workqueue.h>
#include <synch/rcu.h>
#include <errno.h>

#ifdef CONFIG_TEST
#include <test.h>
#endif

/* Data and methods for 'help' command. */
static int cmd_help(cmd_arg_t *argv);
static cmd_info_t help_info = {
	.name = "help",
	.description = "List supported commands.",
	.func = cmd_help,
	.argc = 0
};

/* Data and methods for pio_read_8 command */
static int cmd_pio_read_8(cmd_arg_t *argv);
static cmd_arg_t pio_read_8_argv[] = { { .type = ARG_TYPE_INT } };
static cmd_info_t pio_read_8_info = {
	.name = "pio_read_8",
	.description = "pio_read_8 <address> Read 1 byte from memory (or port).",
	.func = cmd_pio_read_8,
	.argc = 1,
	.argv = pio_read_8_argv
};

/* Data and methods for pio_read_16 command */
static int cmd_pio_read_16(cmd_arg_t *argv);
static cmd_arg_t pio_read_16_argv[] = { { .type = ARG_TYPE_INT } };
static cmd_info_t pio_read_16_info = {
	.name = "pio_read_16",
	.description = "pio_read_16 <address> Read 2 bytes from memory (or port).",
	.func = cmd_pio_read_16,
	.argc = 1,
	.argv = pio_read_16_argv
};

/* Data and methods for pio_read_32 command */
static int cmd_pio_read_32(cmd_arg_t *argv);
static cmd_arg_t pio_read_32_argv[] = { { .type = ARG_TYPE_INT } };
static cmd_info_t pio_read_32_info = {
	.name = "pio_read_32",
	.description = "pio_read_32 <address> Read 4 bytes from memory (or port).",
	.func = cmd_pio_read_32,
	.argc = 1,
	.argv = pio_read_32_argv
};

/* Data and methods for pio_write_8 command */
static int cmd_pio_write_8(cmd_arg_t *argv);
static cmd_arg_t pio_write_8_argv[] = {
	{ .type = ARG_TYPE_INT },
	{ .type = ARG_TYPE_INT }
};
static cmd_info_t pio_write_8_info = {
	.name = "pio_write_8",
	.description = "pio_write_8 <address> <value> Write 1 byte to memory (or port).",
	.func = cmd_pio_write_8,
	.argc = 2,
	.argv = pio_write_8_argv
};

/* Data and methods for pio_write_16 command */
static int cmd_pio_write_16(cmd_arg_t *argv);
static cmd_arg_t pio_write_16_argv[] = {
	{ .type = ARG_TYPE_INT },
	{ .type = ARG_TYPE_INT }
};
static cmd_info_t pio_write_16_info = {
	.name = "pio_write_16",
	.description = "pio_write_16 <address> <value> Write 2 bytes to memory (or port).",
	.func = cmd_pio_write_16,
	.argc = 2,
	.argv = pio_write_16_argv
};

/* Data and methods for pio_write_32 command */
static int cmd_pio_write_32(cmd_arg_t *argv);
static cmd_arg_t pio_write_32_argv[] = {
	{ .type = ARG_TYPE_INT },
	{ .type = ARG_TYPE_INT }
};
static cmd_info_t pio_write_32_info = {
	.name = "pio_write_32",
	.description = "pio_write_32 <address> <value> Write 4 bytes to memory (or port).",
	.func = cmd_pio_write_32,
	.argc = 2,
	.argv = pio_write_32_argv
};

/* Data and methods for 'reboot' command. */
static int cmd_reboot(cmd_arg_t *argv);
static cmd_info_t reboot_info = {
	.name = "reboot",
	.description = "Reboot system.",
	.func = cmd_reboot,
	.argc = 0
};

/* Data and methods for 'uptime' command. */
static int cmd_uptime(cmd_arg_t *argv);
static cmd_info_t uptime_info = {
	.name = "uptime",
	.description = "Show system uptime.",
	.func = cmd_uptime,
	.argc = 0
};

/* Data and methods for 'continue' command. */
static int cmd_continue(cmd_arg_t *argv);
static cmd_info_t continue_info = {
	.name = "continue",
	.description = "Return console back to userspace.",
	.func = cmd_continue,
	.argc = 0
};

#ifdef CONFIG_TEST

/* Data and methods for 'test' command. */
static char test_buf[MAX_CMDLINE + 1];
static int cmd_test(cmd_arg_t *argv);
static cmd_arg_t test_argv[] = {
	{
		.type = ARG_TYPE_STRING_OPTIONAL,
		.buffer = test_buf,
		.len = sizeof(test_buf)
	}
};
static cmd_info_t test_info = {
	.name = "test",
	.description = "<test> List kernel tests or run a test.",
	.func = cmd_test,
	.argc = 1,
	.argv = test_argv,
	.hints_enum = tests_hints_enum
};

/* Data and methods for 'bench' command. */
static int cmd_bench(cmd_arg_t *argv);
static cmd_arg_t bench_argv[] = {
	{
		.type = ARG_TYPE_STRING,
		.buffer = test_buf,
		.len = sizeof(test_buf)
	},
	{
		.type = ARG_TYPE_INT,
	}
};
static cmd_info_t bench_info = {
	.name = "bench",
	.description = "<test> <count> Run kernel test as benchmark.",
	.func = cmd_bench,
	.argc = 2,
	.argv = bench_argv
};

#endif /* CONFIG_TEST */

/* Data and methods for 'description' command. */
static int cmd_desc(cmd_arg_t *argv);
static void desc_help(void);
static char desc_buf[MAX_CMDLINE + 1];
static cmd_arg_t desc_argv = {
	.type = ARG_TYPE_STRING,
	.buffer = desc_buf,
	.len = sizeof(desc_buf)
};
static cmd_info_t desc_info = {
	.name = "describe",
	.description = "<command> Describe specified command.",
	.help = desc_help,
	.func = cmd_desc,
	.argc = 1,
	.argv = &desc_argv,
	.hints_enum = cmdtab_enum
};

/* Data and methods for 'symaddr' command. */
static int cmd_symaddr(cmd_arg_t *argv);
static char symaddr_buf[MAX_CMDLINE + 1];
static cmd_arg_t symaddr_argv = {
	.type = ARG_TYPE_STRING,
	.buffer = symaddr_buf,
	.len = sizeof(symaddr_buf)
};
static cmd_info_t symaddr_info = {
	.name = "symaddr",
	.description = "<symbol> Return symbol address.",
	.func = cmd_symaddr,
	.argc = 1,
	.argv = &symaddr_argv,
	.hints_enum = symtab_hints_enum,
};

/* Data and methods for 'set4' command. */
static char set_buf[MAX_CMDLINE + 1];
static int cmd_set4(cmd_arg_t *argv);
static cmd_arg_t set4_argv[] = {
	{
		.type = ARG_TYPE_STRING,
		.buffer = set_buf,
		.len = sizeof(set_buf)
	},
	{
		.type = ARG_TYPE_INT
	}
};
static cmd_info_t set4_info = {
	.name = "set4",
	.description = "<addr> <value> Set 4B memory location to a value.",
	.func = cmd_set4,
	.argc = 2,
	.argv = set4_argv
};

/* Data and methods for 'call0' and 'mcall0' command. */
static char call0_buf[MAX_CMDLINE + 1];
static char carg1_buf[MAX_CMDLINE + 1];
static char carg2_buf[MAX_CMDLINE + 1];
static char carg3_buf[MAX_CMDLINE + 1];

static int cmd_call0(cmd_arg_t *argv);
static cmd_arg_t call0_argv = {
	.type = ARG_TYPE_STRING,
	.buffer = call0_buf,
	.len = sizeof(call0_buf)
};
static cmd_info_t call0_info = {
	.name = "call0",
	.description = "<function> Call function().",
	.func = cmd_call0,
	.argc = 1,
	.argv = &call0_argv,
	.hints_enum = symtab_hints_enum
};

/* Data and methods for 'mcall0' command. */
static int cmd_mcall0(cmd_arg_t *argv);
static cmd_arg_t mcall0_argv = {
	.type = ARG_TYPE_STRING,
	.buffer = call0_buf,
	.len = sizeof(call0_buf)
};
static cmd_info_t mcall0_info = {
	.name = "mcall0",
	.description = "<function> Call function() on each CPU.",
	.func = cmd_mcall0,
	.argc = 1,
	.argv = &mcall0_argv,
	.hints_enum = symtab_hints_enum
};

/* Data and methods for 'call1' command. */
static int cmd_call1(cmd_arg_t *argv);
static cmd_arg_t call1_argv[] = {
	{
		.type = ARG_TYPE_STRING,
		.buffer = call0_buf,
		.len = sizeof(call0_buf)
	},
	{
		.type = ARG_TYPE_VAR,
		.buffer = carg1_buf,
		.len = sizeof(carg1_buf)
	}
};
static cmd_info_t call1_info = {
	.name = "call1",
	.description = "<function> <arg1> Call function(arg1).",
	.func = cmd_call1,
	.argc = 2,
	.argv = call1_argv,
	.hints_enum = symtab_hints_enum
};

/* Data and methods for 'call2' command. */
static int cmd_call2(cmd_arg_t *argv);
static cmd_arg_t call2_argv[] = {
	{
		.type = ARG_TYPE_STRING,
		.buffer = call0_buf,
		.len = sizeof(call0_buf)
	},
	{
		.type = ARG_TYPE_VAR,
		.buffer = carg1_buf,
		.len = sizeof(carg1_buf)
	},
	{
		.type = ARG_TYPE_VAR,
		.buffer = carg2_buf,
		.len = sizeof(carg2_buf)
	}
};
static cmd_info_t call2_info = {
	.name = "call2",
	.description = "<function> <arg1> <arg2> Call function(arg1, arg2).",
	.func = cmd_call2,
	.argc = 3,
	.argv = call2_argv,
	.hints_enum = symtab_hints_enum
};

/* Data and methods for 'call3' command. */
static int cmd_call3(cmd_arg_t *argv);
static cmd_arg_t call3_argv[] = {
	{
		.type = ARG_TYPE_STRING,
		.buffer = call0_buf,
		.len = sizeof(call0_buf)
	},
	{
		.type = ARG_TYPE_VAR,
		.buffer = carg1_buf,
		.len = sizeof(carg1_buf)
	},
	{
		.type = ARG_TYPE_VAR,
		.buffer = carg2_buf,
		.len = sizeof(carg2_buf)
	},
	{
		.type = ARG_TYPE_VAR,
		.buffer = carg3_buf,
		.len = sizeof(carg3_buf)
	}

};
static cmd_info_t call3_info = {
	.name = "call3",
	.description = "<function> <arg1> <arg2> <arg3> Call function(arg1, arg2, arg3).",
	.func = cmd_call3,
	.argc = 4,
	.argv = call3_argv,
	.hints_enum = symtab_hints_enum
};

/* Data and methods for 'halt' command. */
static int cmd_halt(cmd_arg_t *argv);
static cmd_info_t halt_info = {
	.name = "halt",
	.description = "Halt the kernel.",
	.func = cmd_halt,
	.argc = 0
};

/* Data and methods for 'physmem' command. */
static int cmd_physmem(cmd_arg_t *argv);
cmd_info_t physmem_info = {
	.name = "physmem",
	.description = "Print physical memory configuration.",
	.help = NULL,
	.func = cmd_physmem,
	.argc = 0,
	.argv = NULL
};

/* Data and methods for 'tlb' command. */
static int cmd_tlb(cmd_arg_t *argv);
cmd_info_t tlb_info = {
	.name = "tlb",
	.description = "Print TLB of the current CPU.",
	.help = NULL,
	.func = cmd_tlb,
	.argc = 0,
	.argv = NULL
};

static char flag_buf[MAX_CMDLINE + 1];

static int cmd_threads(cmd_arg_t *argv);
static cmd_arg_t threads_argv = {
	.type = ARG_TYPE_STRING_OPTIONAL,
	.buffer = flag_buf,
	.len = sizeof(flag_buf)
};
static cmd_info_t threads_info = {
	.name = "threads",
	.description = "List all threads (use -a for additional information).",
	.func = cmd_threads,
	.argc = 1,
	.argv = &threads_argv
};

static int cmd_tasks(cmd_arg_t *argv);
static cmd_arg_t tasks_argv = {
	.type = ARG_TYPE_STRING_OPTIONAL,
	.buffer = flag_buf,
	.len = sizeof(flag_buf)
};
static cmd_info_t tasks_info = {
	.name = "tasks",
	.description = "List all tasks (use -a for additional information).",
	.func = cmd_tasks,
	.argc = 1,
	.argv = &tasks_argv
};

#ifdef CONFIG_UDEBUG

/* Data and methods for 'btrace' command */
static int cmd_btrace(cmd_arg_t *argv);
static cmd_arg_t btrace_argv = {
	.type = ARG_TYPE_INT,
};
static cmd_info_t btrace_info = {
	.name = "btrace",
	.description = "<threadid> Show thread stack trace.",
	.func = cmd_btrace,
	.argc = 1,
	.argv = &btrace_argv
};

#endif /* CONFIG_UDEBUG */

static int cmd_sched(cmd_arg_t *argv);
static cmd_info_t sched_info = {
	.name = "scheduler",
	.description = "Show scheduler information.",
	.func = cmd_sched,
	.argc = 0
};

static int cmd_caches(cmd_arg_t *argv);
static cmd_info_t caches_info = {
	.name = "caches",
	.description = "List slab caches.",
	.func = cmd_caches,
	.argc = 0
};

static int cmd_sysinfo(cmd_arg_t *argv);
static cmd_info_t sysinfo_info = {
	.name = "sysinfo",
	.description = "Dump sysinfo.",
	.func = cmd_sysinfo,
	.argc = 0
};

/* Data and methods for 'zones' command */
static int cmd_zones(cmd_arg_t *argv);
static cmd_info_t zones_info = {
	.name = "zones",
	.description = "List memory zones.",
	.func = cmd_zones,
	.argc = 0
};

/* Data and methods for 'zone' command */
static int cmd_zone(cmd_arg_t *argv);
static cmd_arg_t zone_argv = {
	.type = ARG_TYPE_INT,
};

static cmd_info_t zone_info = {
	.name = "zone",
	.description = "<zone> Show memory zone structure.",
	.func = cmd_zone,
	.argc = 1,
	.argv = &zone_argv
};

/* Data and methods for the 'workq' command */
static int cmd_workq(cmd_arg_t *argv);
static cmd_info_t workq_info = {
	.name = "workq",
	.description = "Show global workq information.",
	.func = cmd_workq,
	.argc = 0
};

/* Data and methods for the 'workq' command */
static int cmd_rcu(cmd_arg_t *argv);
static cmd_info_t rcu_info = {
	.name = "rcu",
	.description = "Show RCU run-time statistics.",
	.func = cmd_rcu,
	.argc = 0
};

/* Data and methods for 'ipc' command */
static int cmd_ipc(cmd_arg_t *argv);
static cmd_arg_t ipc_argv = {
	.type = ARG_TYPE_INT,
};
static cmd_info_t ipc_info = {
	.name = "ipc",
	.description = "<taskid> Show IPC information of a task.",
	.func = cmd_ipc,
	.argc = 1,
	.argv = &ipc_argv
};

/* Data and methods for 'kill' command */
static int cmd_kill(cmd_arg_t *argv);
static cmd_arg_t kill_argv = {
	.type = ARG_TYPE_INT,
};
static cmd_info_t kill_info = {
	.name = "kill",
	.description = "<taskid> Kill a task.",
	.func = cmd_kill,
	.argc = 1,
	.argv = &kill_argv
};

/* Data and methods for 'cpus' command. */
static int cmd_cpus(cmd_arg_t *argv);
cmd_info_t cpus_info = {
	.name = "cpus",
	.description = "List all processors.",
	.help = NULL,
	.func = cmd_cpus,
	.argc = 0,
	.argv = NULL
};

/* Data and methods for 'version' command. */
static int cmd_version(cmd_arg_t *argv);
cmd_info_t version_info = {
	.name = "version",
	.description = "Print version information.",
	.help = NULL,
	.func = cmd_version,
	.argc = 0,
	.argv = NULL
};

static cmd_info_t *basic_commands[] = {
	&call0_info,
	&mcall0_info,
	&caches_info,
	&call1_info,
	&call2_info,
	&call3_info,
	&continue_info,
	&cpus_info,
	&desc_info,
	&halt_info,
	&help_info,
	&ipc_info,
	&kill_info,
	&physmem_info,
	&reboot_info,
	&rcu_info,
	&sched_info,
	&set4_info,
	&symaddr_info,
	&sysinfo_info,
	&tasks_info,
	&threads_info,
	&tlb_info,
	&uptime_info,
	&version_info,
	&workq_info,
	&zones_info,
	&zone_info,
#ifdef CONFIG_TEST
	&test_info,
	&bench_info,
#endif
#ifdef CONFIG_UDEBUG
	&btrace_info,
#endif
	&pio_read_8_info,
	&pio_read_16_info,
	&pio_read_32_info,
	&pio_write_8_info,
	&pio_write_16_info,
	&pio_write_32_info,
	NULL
};


/** Initialize command info structure.
 *
 * @param cmd Command info structure.
 *
 */
void cmd_initialize(cmd_info_t *cmd)
{
	spinlock_initialize(&cmd->lock, "cmd.lock");
	link_initialize(&cmd->link);
}

/** Initialize and register commands. */
void cmd_init(void)
{
	unsigned int i;

	for (i = 0; basic_commands[i]; i++) {
		cmd_initialize(basic_commands[i]);
	}

	for (i = 0; basic_commands[i]; i++) {
		if (!cmd_register(basic_commands[i])) {
			log(LF_OTHER, LVL_ERROR,
			    "Cannot register command %s",
			    basic_commands[i]->name);
		}
	}
}

/** List supported commands.
 *
 * @param argv Argument vector.
 *
 * @return 0 on failure, 1 on success.
 */
int cmd_help(cmd_arg_t *argv)
{
	spinlock_lock(&cmd_lock);

	size_t len = 0;
	list_foreach(cmd_list, link, cmd_info_t, hlp) {
		spinlock_lock(&hlp->lock);
		if (str_length(hlp->name) > len)
			len = str_length(hlp->name);
		spinlock_unlock(&hlp->lock);
	}

	unsigned int _len = (unsigned int) len;
	if ((_len != len) || (((int) _len) < 0)) {
		log(LF_OTHER, LVL_ERROR, "Command length overflow");
		return 1;
	}

	list_foreach(cmd_list, link, cmd_info_t, hlp) {
		spinlock_lock(&hlp->lock);
		printf("%-*s %s\n", _len, hlp->name, hlp->description);
		spinlock_unlock(&hlp->lock);
	}

	spinlock_unlock(&cmd_lock);

	return 1;
}

/** Read 1 byte from phys memory or io port.
 *
 * @param argv Argument vector.
 *
 * @return 0 on failure, 1 on success.
 */
static int cmd_pio_read_8(cmd_arg_t *argv)
{
	uint8_t *ptr = NULL;

#ifdef IO_SPACE_BOUNDARY
	if ((void *) argv->intval < IO_SPACE_BOUNDARY)
		ptr = (void *) argv[0].intval;
	else
#endif
		ptr = (uint8_t *) km_map(argv[0].intval, sizeof(uint8_t),
		    PAGE_NOT_CACHEABLE);

	const uint8_t val = pio_read_8(ptr);
	printf("read %" PRIxn ": %" PRIx8 "\n", argv[0].intval, val);

#ifdef IO_SPACE_BOUNDARY
	if ((void *) argv->intval < IO_SPACE_BOUNDARY)
		return 1;
#endif

	km_unmap((uintptr_t) ptr, sizeof(uint8_t));
	return 1;
}

/** Read 2 bytes from phys memory or io port.
 *
 * @param argv Argument vector.
 *
 * @return 0 on failure, 1 on success.
 */
static int cmd_pio_read_16(cmd_arg_t *argv)
{
	uint16_t *ptr = NULL;

#ifdef IO_SPACE_BOUNDARY
	if ((void *) argv->intval < IO_SPACE_BOUNDARY)
		ptr = (void *) argv[0].intval;
	else
#endif
		ptr = (uint16_t *) km_map(argv[0].intval, sizeof(uint16_t),
		    PAGE_NOT_CACHEABLE);

	const uint16_t val = pio_read_16(ptr);
	printf("read %" PRIxn ": %" PRIx16 "\n", argv[0].intval, val);

#ifdef IO_SPACE_BOUNDARY
	if ((void *) argv->intval < IO_SPACE_BOUNDARY)
		return 1;
#endif

	km_unmap((uintptr_t) ptr, sizeof(uint16_t));
	return 1;
}

/** Read 4 bytes from phys memory or io port.
 *
 * @param argv Argument vector.
 *
 * @return 0 on failure, 1 on success.
 */
static int cmd_pio_read_32(cmd_arg_t *argv)
{
	uint32_t *ptr = NULL;

#ifdef IO_SPACE_BOUNDARY
	if ((void *) argv->intval < IO_SPACE_BOUNDARY)
		ptr = (void *) argv[0].intval;
	else
#endif
		ptr = (uint32_t *) km_map(argv[0].intval, sizeof(uint32_t),
		    PAGE_NOT_CACHEABLE);

	const uint32_t val = pio_read_32(ptr);
	printf("read %" PRIxn ": %" PRIx32 "\n", argv[0].intval, val);

#ifdef IO_SPACE_BOUNDARY
	if ((void *) argv->intval < IO_SPACE_BOUNDARY)
		return 1;
#endif

	km_unmap((uintptr_t) ptr, sizeof(uint32_t));
	return 1;
}

/** Write 1 byte to phys memory or io port.
 *
 * @param argv Argument vector.
 *
 * @return 0 on failure, 1 on success.
 */
static int cmd_pio_write_8(cmd_arg_t *argv)
{
	uint8_t *ptr = NULL;

#ifdef IO_SPACE_BOUNDARY
	if ((void *) argv->intval < IO_SPACE_BOUNDARY)
		ptr = (void *) argv[0].intval;
	else
#endif
		ptr = (uint8_t *) km_map(argv[0].intval, sizeof(uint8_t),
		    PAGE_NOT_CACHEABLE);

	printf("write %" PRIxn ": %" PRIx8 "\n", argv[0].intval,
	    (uint8_t) argv[1].intval);
	pio_write_8(ptr, (uint8_t) argv[1].intval);

#ifdef IO_SPACE_BOUNDARY
	if ((void *) argv->intval < IO_SPACE_BOUNDARY)
		return 1;
#endif

	km_unmap((uintptr_t) ptr, sizeof(uint8_t));
	return 1;
}

/** Write 2 bytes to phys memory or io port.
 *
 * @param argv Argument vector.
 *
 * @return 0 on failure, 1 on success.
 */
static int cmd_pio_write_16(cmd_arg_t *argv)
{
	uint16_t *ptr = NULL;

#ifdef IO_SPACE_BOUNDARY
	if ((void *) argv->intval < IO_SPACE_BOUNDARY)
		ptr = (void *) argv[0].intval;
	else
#endif
		ptr = (uint16_t *) km_map(argv[0].intval, sizeof(uint16_t),
		    PAGE_NOT_CACHEABLE);

	printf("write %" PRIxn ": %" PRIx16 "\n", argv[0].intval,
	    (uint16_t) argv[1].intval);
	pio_write_16(ptr, (uint16_t) argv[1].intval);

#ifdef IO_SPACE_BOUNDARY
	if ((void *) argv->intval < IO_SPACE_BOUNDARY)
		return 1;
#endif

	km_unmap((uintptr_t) ptr, sizeof(uint16_t));
	return 1;
}

/** Write 4 bytes to phys memory or io port.
 *
 * @param argv Argument vector.
 *
 * @return 0 on failure, 1 on success.
 */
static int cmd_pio_write_32(cmd_arg_t *argv)
{
	uint32_t *ptr = NULL;

#ifdef IO_SPACE_BOUNDARY
	if ((void *) argv->intval < IO_SPACE_BOUNDARY)
		ptr = (void *) argv[0].intval;
	else
#endif
		ptr = (uint32_t *) km_map(argv[0].intval, sizeof(uint32_t),
		    PAGE_NOT_CACHEABLE);

	printf("write %" PRIxn ": %" PRIx32 "\n", argv[0].intval,
	    (uint32_t) argv[1].intval);
	pio_write_32(ptr, (uint32_t) argv[1].intval);

#ifdef IO_SPACE_BOUNDARY
	if ((void *) argv->intval < IO_SPACE_BOUNDARY)
		return 1;
#endif

	km_unmap((uintptr_t) ptr, sizeof(uint32_t));
	return 1;
}

/** Reboot the system.
 *
 * @param argv Argument vector.
 *
 * @return 0 on failure, 1 on success.
 */
int cmd_reboot(cmd_arg_t *argv)
{
	reboot();

	/* Not reached */
	return 1;
}

/** Print system uptime information.
 *
 * @param argv Argument vector.
 *
 * @return 0 on failure, 1 on success.
 */
int cmd_uptime(cmd_arg_t *argv)
{
	assert(uptime);

	/* This doesn't have to be very accurate */
	sysarg_t sec = uptime->seconds1;

	printf("Up %" PRIun " days, %" PRIun " hours, %" PRIun " minutes, %" PRIun " seconds\n",
		sec / 86400, (sec % 86400) / 3600, (sec % 3600) / 60, sec % 60);

	return 1;
}

/** Describe specified command.
 *
 * @param argv Argument vector.
 *
 * @return 0 on failure, 1 on success.
 */
int cmd_desc(cmd_arg_t *argv)
{
	spinlock_lock(&cmd_lock);

	list_foreach(cmd_list, link, cmd_info_t, hlp) {
		spinlock_lock(&hlp->lock);

		if (str_lcmp(hlp->name, (const char *) argv->buffer, str_length(hlp->name)) == 0) {
			printf("%s - %s\n", hlp->name, hlp->description);
			if (hlp->help)
				hlp->help();
			spinlock_unlock(&hlp->lock);
			break;
		}

		spinlock_unlock(&hlp->lock);
	}

	spinlock_unlock(&cmd_lock);

	return 1;
}

/** Search symbol table */
int cmd_symaddr(cmd_arg_t *argv)
{
	symtab_print_search((char *) argv->buffer);

	return 1;
}

/** Call function with zero parameters */
int cmd_call0(cmd_arg_t *argv)
{
	uintptr_t symaddr;
	char *symbol;
	sysarg_t (*fnc)(void);
	fncptr_t fptr;
	errno_t rc;

	symbol = (char *) argv->buffer;
	rc = symtab_addr_lookup(symbol, &symaddr);

	if (rc == ENOENT)
		printf("Symbol %s not found.\n", symbol);
	else if (rc == EOVERFLOW) {
		symtab_print_search(symbol);
		printf("Duplicate symbol, be more specific.\n");
	} else if (rc == EOK) {
		ipl_t ipl;

		ipl = interrupts_disable();
		fnc = (sysarg_t (*)(void)) arch_construct_function(&fptr,
		    (void *) symaddr, (void *) cmd_call0);
		printf("Calling %s() (%p)\n", symbol, (void *) symaddr);
		printf("Result: %#" PRIxn "\n", fnc());
		interrupts_restore(ipl);
	} else {
		printf("No symbol information available.\n");
	}
	return 1;
}

/** Call function with zero parameters on each CPU */
int cmd_mcall0(cmd_arg_t *argv)
{
	/*
	 * For each CPU, create a thread which will
	 * call the function.
	 */

	unsigned int i;
	for (i = 0; i < config.cpu_count; i++) {
		if (!cpus[i].active)
			continue;

		thread_t *thread;
		if ((thread = thread_create((void (*)(void *)) cmd_call0,
		    (void *) argv, TASK, THREAD_FLAG_NONE, "call0"))) {
			printf("cpu%u: ", i);
			thread_wire(thread, &cpus[i]);
			thread_ready(thread);
			thread_join(thread);
			thread_detach(thread);
		} else
			printf("Unable to create thread for cpu%u\n", i);
	}

	return 1;
}

/** Call function with one parameter */
int cmd_call1(cmd_arg_t *argv)
{
	uintptr_t symaddr;
	char *symbol;
	sysarg_t (*fnc)(sysarg_t, ...);
	sysarg_t arg1 = argv[1].intval;
	fncptr_t fptr;
	errno_t rc;

	symbol = (char *) argv->buffer;
	rc = symtab_addr_lookup(symbol, &symaddr);

	if (rc == ENOENT) {
		printf("Symbol %s not found.\n", symbol);
	} else if (rc == EOVERFLOW) {
		symtab_print_search(symbol);
		printf("Duplicate symbol, be more specific.\n");
	} else if (rc == EOK) {
		ipl_t ipl;

		ipl = interrupts_disable();
		fnc = (sysarg_t (*)(sysarg_t, ...))
		    arch_construct_function(&fptr, (void *) symaddr,
		    (void *) cmd_call1);
		printf("Calling f(%#" PRIxn "): %p: %s\n", arg1,
		    (void *) symaddr, symbol);
		printf("Result: %#" PRIxn "\n", fnc(arg1));
		interrupts_restore(ipl);
	} else {
		printf("No symbol information available.\n");
	}

	return 1;
}

/** Call function with two parameters */
int cmd_call2(cmd_arg_t *argv)
{
	uintptr_t symaddr;
	char *symbol;
	sysarg_t (*fnc)(sysarg_t, sysarg_t, ...);
	sysarg_t arg1 = argv[1].intval;
	sysarg_t arg2 = argv[2].intval;
	fncptr_t fptr;
	errno_t rc;

	symbol = (char *) argv->buffer;
	rc = symtab_addr_lookup(symbol, &symaddr);

	if (rc == ENOENT) {
		printf("Symbol %s not found.\n", symbol);
	} else if (rc == EOVERFLOW) {
		symtab_print_search(symbol);
		printf("Duplicate symbol, be more specific.\n");
	} else if (rc == EOK) {
		ipl_t ipl;

		ipl = interrupts_disable();
		fnc = (sysarg_t (*)(sysarg_t, sysarg_t, ...))
		    arch_construct_function(&fptr, (void *) symaddr,
		    (void *) cmd_call2);
		printf("Calling f(%#" PRIxn ", %#" PRIxn "): %p: %s\n",
		       arg1, arg2, (void *) symaddr, symbol);
		printf("Result: %#" PRIxn "\n", fnc(arg1, arg2));
		interrupts_restore(ipl);
	} else {
		printf("No symbol information available.\n");
	}
	return 1;
}

/** Call function with three parameters */
int cmd_call3(cmd_arg_t *argv)
{
	uintptr_t symaddr;
	char *symbol;
	sysarg_t (*fnc)(sysarg_t, sysarg_t, sysarg_t, ...);
	sysarg_t arg1 = argv[1].intval;
	sysarg_t arg2 = argv[2].intval;
	sysarg_t arg3 = argv[3].intval;
	fncptr_t fptr;
	errno_t rc;

	symbol = (char *) argv->buffer;
	rc = symtab_addr_lookup(symbol, &symaddr);

	if (rc == ENOENT) {
		printf("Symbol %s not found.\n", symbol);
	} else if (rc == EOVERFLOW) {
		symtab_print_search(symbol);
		printf("Duplicate symbol, be more specific.\n");
	} else if (rc == EOK) {
		ipl_t ipl;

		ipl = interrupts_disable();
		fnc = (sysarg_t (*)(sysarg_t, sysarg_t, sysarg_t, ...))
		    arch_construct_function(&fptr, (void *) symaddr,
		    (void *) cmd_call3);
		printf("Calling f(%#" PRIxn ",%#" PRIxn ", %#" PRIxn "): %p: %s\n",
		       arg1, arg2, arg3, (void *) symaddr, symbol);
		printf("Result: %#" PRIxn "\n", fnc(arg1, arg2, arg3));
		interrupts_restore(ipl);
	} else {
		printf("No symbol information available.\n");
	}
	return 1;
}

/** Print detailed description of 'describe' command. */
void desc_help(void)
{
	printf("Syntax: describe command_name\n");
}

/** Halt the kernel.
 *
 * @param argv Argument vector (ignored).
 *
 * @return 0 on failure, 1 on success (never returns).
 */
int cmd_halt(cmd_arg_t *argv)
{
	halt();
	return 1;
}

/** Command for printing TLB contents.
 *
 * @param argv Not used.
 *
 * @return Always returns 1.
 */
int cmd_tlb(cmd_arg_t *argv)
{
	tlb_print();
	return 1;
}

/** Command for printing physical memory configuration.
 *
 * @param argv Not used.
 *
 * @return Always returns 1.
 */
int cmd_physmem(cmd_arg_t *argv)
{
	physmem_print();
	return 1;
}

/** Write 4 byte value to address */
int cmd_set4(cmd_arg_t *argv)
{
	uintptr_t addr = 0; // Prevent -Werror=maybe-uninitialized
	uint32_t arg1 = argv[1].intval;
	bool pointer = false;
	errno_t rc;

	if (((char *) argv->buffer)[0] == '*') {
		rc = symtab_addr_lookup((char *) argv->buffer + 1, &addr);
		pointer = true;
	} else if (((char *) argv->buffer)[0] >= '0' &&
		   ((char *) argv->buffer)[0] <= '9') {
		uint64_t value;
		rc = str_uint64_t((char *) argv->buffer, NULL, 0, true, &value);
		if (rc == EOK)
			addr = (uintptr_t) value;
	} else
		rc = symtab_addr_lookup((char *) argv->buffer, &addr);

	if (rc == ENOENT)
		printf("Symbol %s not found.\n", (char *) argv->buffer);
	else if (rc == EINVAL)
		printf("Invalid address.\n");
	else if (rc == EOVERFLOW) {
		symtab_print_search((char *) argv->buffer);
		printf("Duplicate symbol (be more specific) or address overflow.\n");
	} else if (rc == EOK) {
		if (pointer)
			addr = *(uintptr_t *) addr;
		printf("Writing %#" PRIx32" -> %p\n", arg1, (void *) addr);
		*(uint32_t *) addr = arg1;
	} else
		printf("No symbol information available.\n");

	return 1;
}

/** Command for listing slab allocator caches
 *
 * @param argv Ignored
 *
 * @return Always 1
 */
int cmd_caches(cmd_arg_t *argv)
{
	slab_print_list();
	return 1;
}

/** Command for dumping sysinfo
 *
 * @param argv Ignores
 *
 * @return Always 1
 */
int cmd_sysinfo(cmd_arg_t *argv)
{
	sysinfo_dump(NULL);
	return 1;
}

/** Command for listing thread information
 *
 * @param argv Ignored
 *
 * @return Always 1
 */
int cmd_threads(cmd_arg_t *argv)
{
	if (str_cmp(flag_buf, "-a") == 0)
		thread_print_list(true);
	else if (str_cmp(flag_buf, "") == 0)
		thread_print_list(false);
	else
		printf("Unknown argument \"%s\".\n", flag_buf);

	return 1;
}

/** Command for listing task information
 *
 * @param argv Ignored
 *
 * @return Always 1
 */
int cmd_tasks(cmd_arg_t *argv)
{
	if (str_cmp(flag_buf, "-a") == 0)
		task_print_list(true);
	else if (str_cmp(flag_buf, "") == 0)
		task_print_list(false);
	else
		printf("Unknown argument \"%s\".\n", flag_buf);

	return 1;
}

#ifdef CONFIG_UDEBUG

/** Command for printing thread stack trace
 *
 * @param argv Integer argument from cmdline expected
 *
 * return Always 1
 *
 */
int cmd_btrace(cmd_arg_t *argv)
{
	thread_stack_trace(argv[0].intval);
	return 1;
}

#endif /* CONFIG_UDEBUG */

/** Command for printing scheduler information
 *
 * @param argv Ignores
 *
 * @return Always 1
 */
int cmd_sched(cmd_arg_t *argv)
{
	sched_print_list();
	return 1;
}

/** Prints information about the global work queue.
 *
 * @param argv Ignores
 *
 * @return Always 1
 */
int cmd_workq(cmd_arg_t *argv)
{
	workq_global_print_info();
	return 1;
}

/** Prints RCU statistics.
 *
 * @param argv Ignores
 *
 * @return Always 1
 */
int cmd_rcu(cmd_arg_t *argv)
{
	rcu_print_stat();
	return 1;
}

/** Command for listing memory zones
 *
 * @param argv Ignored
 *
 * return Always 1
 */
int cmd_zones(cmd_arg_t *argv)
{
	zones_print_list();
	return 1;
}

/** Command for memory zone details
 *
 * @param argv Integer argument from cmdline expected
 *
 * return Always 1
 */
int cmd_zone(cmd_arg_t *argv)
{
	zone_print_one(argv[0].intval);
	return 1;
}

/** Command for printing task IPC details
 *
 * @param argv Integer argument from cmdline expected
 *
 * return Always 1
 */
int cmd_ipc(cmd_arg_t *argv)
{
	ipc_print_task(argv[0].intval);
	return 1;
}

/** Command for killing a task
 *
 * @param argv Integer argument from cmdline expected
 *
 * return 0 on failure, 1 on success.
 */
int cmd_kill(cmd_arg_t *argv)
{
	if (task_kill(argv[0].intval) != EOK)
		return 0;

	return 1;
}

/** Command for listing processors.
 *
 * @param argv Ignored.
 *
 * return Always 1.
 */
int cmd_cpus(cmd_arg_t *argv)
{
	cpu_list();
	return 1;
}

/** Command for printing kernel version.
 *
 * @param argv Ignored.
 *
 * return Always 1.
 */
int cmd_version(cmd_arg_t *argv)
{
	version_print();
	return 1;
}

/** Command for returning console back to userspace.
 *
 * @param argv Ignored.
 *
 * return Always 1.
 */
int cmd_continue(cmd_arg_t *argv)
{
	printf("The kernel will now relinquish the console.\n");
	release_console();
	indev_pop_character(stdin);

	return 1;
}

#ifdef CONFIG_TEST
static bool run_test(const test_t *test)
{
	printf("%s (%s)\n", test->name, test->desc);

	/* Update and read thread accounting
	   for benchmarking */
	irq_spinlock_lock(&TASK->lock, true);
	uint64_t ucycles0, kcycles0;
	task_get_accounting(TASK, &ucycles0, &kcycles0);
	irq_spinlock_unlock(&TASK->lock, true);

	/* Execute the test */
	test_quiet = false;
	const char *ret = test->entry();

	/* Update and read thread accounting */
	uint64_t ucycles1, kcycles1;
	irq_spinlock_lock(&TASK->lock, true);
	task_get_accounting(TASK, &ucycles1, &kcycles1);
	irq_spinlock_unlock(&TASK->lock, true);

	uint64_t ucycles, kcycles;
	char usuffix, ksuffix;
	order_suffix(ucycles1 - ucycles0, &ucycles, &usuffix);
	order_suffix(kcycles1 - kcycles0, &kcycles, &ksuffix);

	printf("Time: %" PRIu64 "%c user cycles, %" PRIu64 "%c kernel cycles\n",
	    ucycles, usuffix, kcycles, ksuffix);

	if (ret == NULL) {
		printf("Test passed\n");
		return true;
	}

	printf("%s\n", ret);
	return false;
}

static bool run_bench(const test_t *test, const uint32_t cnt)
{
	uint32_t i;
	bool ret = true;
	uint64_t ucycles, kcycles;
	char usuffix, ksuffix;

	if (cnt < 1)
		return true;

	uint64_t *data = (uint64_t *) malloc(sizeof(uint64_t) * cnt, 0);
	if (data == NULL) {
		printf("Error allocating memory for statistics\n");
		return false;
	}

	for (i = 0; i < cnt; i++) {
		printf("%s (%u/%u) ... ", test->name, i + 1, cnt);

		/* Update and read thread accounting
		   for benchmarking */
		irq_spinlock_lock(&TASK->lock, true);
		uint64_t ucycles0, kcycles0;
		task_get_accounting(TASK, &ucycles0, &kcycles0);
		irq_spinlock_unlock(&TASK->lock, true);

		/* Execute the test */
		test_quiet = true;
		const char *test_ret = test->entry();

		/* Update and read thread accounting */
		irq_spinlock_lock(&TASK->lock, true);
		uint64_t ucycles1, kcycles1;
		task_get_accounting(TASK, &ucycles1, &kcycles1);
		irq_spinlock_unlock(&TASK->lock, true);

		if (test_ret != NULL) {
			printf("%s\n", test_ret);
			ret = false;
			break;
		}

		data[i] = ucycles1 - ucycles0 + kcycles1 - kcycles0;
		order_suffix(ucycles1 - ucycles0, &ucycles, &usuffix);
		order_suffix(kcycles1 - kcycles0, &kcycles, &ksuffix);
		printf("OK (%" PRIu64 "%c user cycles, %" PRIu64 "%c kernel cycles)\n",
		    ucycles, usuffix, kcycles, ksuffix);
	}

	if (ret) {
		printf("\n");

		uint64_t sum = 0;

		for (i = 0; i < cnt; i++) {
			sum += data[i];
		}

		order_suffix(sum / (uint64_t) cnt, &ucycles, &usuffix);
		printf("Average\t\t%" PRIu64 "%c\n", ucycles, usuffix);
	}

	free(data);

	return ret;
}

static void list_tests(void)
{
	size_t len = 0;
	test_t *test;

	for (test = tests; test->name != NULL; test++) {
		if (str_length(test->name) > len)
			len = str_length(test->name);
	}

	unsigned int _len = (unsigned int) len;
	if ((_len != len) || (((int) _len) < 0)) {
		printf("Command length overflow\n");
		return;
	}

	for (test = tests; test->name != NULL; test++)
		printf("%-*s %s%s\n", _len, test->name, test->desc,
		    (test->safe ? "" : " (unsafe)"));

	printf("%-*s Run all safe tests\n", _len, "*");
}

/** Command for listing and running kernel tests
 *
 * @param argv Argument vector.
 *
 * return Always 1.
 *
 */
int cmd_test(cmd_arg_t *argv)
{
	test_t *test;

	if (str_cmp((char *) argv->buffer, "*") == 0) {
		for (test = tests; test->name != NULL; test++) {
			if (test->safe) {
				printf("\n");
				if (!run_test(test))
					break;
			}
		}
	} else if (str_cmp((char *) argv->buffer, "") != 0) {
		bool fnd = false;

		for (test = tests; test->name != NULL; test++) {
			if (str_cmp(test->name, (char *) argv->buffer) == 0) {
				fnd = true;
				run_test(test);
				break;
			}
		}

		if (!fnd)
			printf("Unknown test\n");
	} else
		list_tests();

	return 1;
}

/** Command for returning kernel tests as benchmarks
 *
 * @param argv Argument vector.
 *
 * return Always 1.
 */
int cmd_bench(cmd_arg_t *argv)
{
	test_t *test;
	uint32_t cnt = argv[1].intval;

	if (str_cmp((char *) argv->buffer, "*") == 0) {
		for (test = tests; test->name != NULL; test++) {
			if (test->safe) {
				if (!run_bench(test, cnt))
					break;
			}
		}
	} else {
		bool fnd = false;

		for (test = tests; test->name != NULL; test++) {
			if (str_cmp(test->name, (char *) argv->buffer) == 0) {
				fnd = true;

				if (test->safe)
					run_bench(test, cnt);
				else
					printf("Unsafe test\n");

				break;
			}
		}

		if (!fnd)
			printf("Unknown test\n");
	}

	return 1;
}

#endif

/** @}
 */
