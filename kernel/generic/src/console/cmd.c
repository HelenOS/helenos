/*
 * Copyright (C) 2005 Jakub Jermar
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
 * @file	cmd.c
 * @brief	Kernel console command wrappers.
 *
 * This file is meant to contain all wrapper functions for
 * all kconsole commands. The point is in separating
 * kconsole specific wrappers from kconsole-unaware functions
 * from other subsystems.
 */

#include <console/cmd.h>
#include <console/console.h>
#include <console/kconsole.h>
#include <print.h>
#include <panic.h>
#include <typedefs.h>
#include <arch/types.h>
#include <adt/list.h>
#include <arch.h>
#include <func.h>
#include <macros.h>
#include <debug.h>
#include <symtab.h>
#include <cpu.h>
#include <mm/tlb.h>
#include <arch/mm/tlb.h>
#include <mm/frame.h>
#include <main/version.h>
#include <mm/slab.h>
#include <proc/scheduler.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <ipc/ipc.h>
#include <ipc/irq.h>

#ifdef CONFIG_TEST
#include <test.h>
#endif

/* Data and methods for 'help' command. */
static int cmd_help(cmd_arg_t *argv);
static cmd_info_t help_info = {
	.name = "help",
	.description = "List of supported commands.",
	.func = cmd_help,
	.argc = 0
};

static cmd_info_t exit_info = {
	.name = "exit",
	.description = "Exit kconsole",
	.argc = 0
};

static int cmd_continue(cmd_arg_t *argv);
static cmd_info_t continue_info = {
	.name = "continue",
	.description = "Return console back to userspace.",
	.func = cmd_continue,
	.argc = 0
};

#ifdef CONFIG_TEST
static int cmd_tests(cmd_arg_t *argv);
static cmd_info_t tests_info = {
	.name = "tests",
	.description = "Print available kernel tests.",
	.func = cmd_tests,
	.argc = 0
};

static char test_buf[MAX_CMDLINE + 1];
static int cmd_test(cmd_arg_t *argv);
static cmd_arg_t test_argv[] = {
	{
		.type = ARG_TYPE_STRING,
		.buffer = test_buf,
		.len = sizeof(test_buf)
	}
};
static cmd_info_t test_info = {
	.name = "test",
	.description = "Run kernel test.",
	.func = cmd_test,
	.argc = 1,
	.argv = test_argv
};
#endif

/* Data and methods for 'description' command. */
static int cmd_desc(cmd_arg_t *argv);
static void desc_help(void);
static char desc_buf[MAX_CMDLINE+1];
static cmd_arg_t desc_argv = {
	.type = ARG_TYPE_STRING,
	.buffer = desc_buf,
	.len = sizeof(desc_buf)
};
static cmd_info_t desc_info = {
	.name = "describe",
	.description = "Describe specified command.",
	.help = desc_help,
	.func = cmd_desc,
	.argc = 1,
	.argv = &desc_argv
};

/* Data and methods for 'symaddr' command. */
static int cmd_symaddr(cmd_arg_t *argv);
static char symaddr_buf[MAX_CMDLINE+1];
static cmd_arg_t symaddr_argv = {
	.type = ARG_TYPE_STRING,
	.buffer = symaddr_buf,
	.len = sizeof(symaddr_buf)
};
static cmd_info_t symaddr_info = {
	.name = "symaddr",
	.description = "Return symbol address.",
	.func = cmd_symaddr,
	.argc = 1,
	.argv = &symaddr_argv
};

static char set_buf[MAX_CMDLINE+1];
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
	.description = "set <dest_addr> <value> - 4byte version",
	.func = cmd_set4,
	.argc = 2,
	.argv = set4_argv
};

/* Data and methods for 'call0' command. */
static char call0_buf[MAX_CMDLINE+1];
static char carg1_buf[MAX_CMDLINE+1];
static char carg2_buf[MAX_CMDLINE+1];
static char carg3_buf[MAX_CMDLINE+1];

static int cmd_call0(cmd_arg_t *argv);
static cmd_arg_t call0_argv = {
	.type = ARG_TYPE_STRING,
	.buffer = call0_buf,
	.len = sizeof(call0_buf)
};
static cmd_info_t call0_info = {
	.name = "call0",
	.description = "call0 <function> -> call function().",
	.func = cmd_call0,
	.argc = 1,
	.argv = &call0_argv
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
	.description = "call1 <function> <arg1> -> call function(arg1).",
	.func = cmd_call1,
	.argc = 2,
	.argv = call1_argv
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
	.description = "call2 <function> <arg1> <arg2> -> call function(arg1,arg2).",
	.func = cmd_call2,
	.argc = 3,
	.argv = call2_argv
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
	.description = "call3 <function> <arg1> <arg2> <arg3> -> call function(arg1,arg2,arg3).",
	.func = cmd_call3,
	.argc = 4,
	.argv = call3_argv
};

/* Data and methods for 'halt' command. */
static int cmd_halt(cmd_arg_t *argv);
static cmd_info_t halt_info = {
	.name = "halt",
	.description = "Halt the kernel.",
	.func = cmd_halt,
	.argc = 0
};

/* Data and methods for 'tlb' command. */
static int cmd_tlb(cmd_arg_t *argv);
cmd_info_t tlb_info = {
	.name = "tlb",
	.description = "Print TLB of current processor.",
	.help = NULL,
	.func = cmd_tlb,
	.argc = 0,
	.argv = NULL
};

static int cmd_threads(cmd_arg_t *argv);
static cmd_info_t threads_info = {
	.name = "threads",
	.description = "List all threads.",
	.func = cmd_threads,
	.argc = 0
};

static int cmd_tasks(cmd_arg_t *argv);
static cmd_info_t tasks_info = {
	.name = "tasks",
	.description = "List all tasks.",
	.func = cmd_tasks,
	.argc = 0
};


static int cmd_sched(cmd_arg_t *argv);
static cmd_info_t sched_info = {
	.name = "scheduler",
	.description = "List all scheduler information.",
	.func = cmd_sched,
	.argc = 0
};

static int cmd_slabs(cmd_arg_t *argv);
static cmd_info_t slabs_info = {
	.name = "slabs",
	.description = "List slab caches.",
	.func = cmd_slabs,
	.argc = 0
};

/* Data and methods for 'zones' command */
static int cmd_zones(cmd_arg_t *argv);
static cmd_info_t zones_info = {
	.name = "zones",
	.description = "List of memory zones.",
	.func = cmd_zones,
	.argc = 0
};

/* Data and methods for 'ipc_task' command */
static int cmd_ipc_task(cmd_arg_t *argv);
static cmd_arg_t ipc_task_argv = {
	.type = ARG_TYPE_INT,
};
static cmd_info_t ipc_task_info = {
	.name = "ipc_task",
	.description = "ipc_task <taskid> Show IPC information of given task.",
	.func = cmd_ipc_task,
	.argc = 1,
	.argv = &ipc_task_argv
};

/* Data and methods for 'zone' command */
static int cmd_zone(cmd_arg_t *argv);
static cmd_arg_t zone_argv = {
	.type = ARG_TYPE_INT,
};

static cmd_info_t zone_info = {
	.name = "zone",
	.description = "Show memory zone structure.",
	.func = cmd_zone,
	.argc = 1,
	.argv = &zone_argv
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
	&call1_info,
	&call2_info,
	&call3_info,
	&continue_info,
	&cpus_info,
	&desc_info,
	&exit_info,
	&halt_info,
	&help_info,
	&ipc_task_info,
	&set4_info,
	&slabs_info,
	&symaddr_info,
	&sched_info,
	&threads_info,
	&tasks_info,
	&tlb_info,
	&version_info,
	&zones_info,
	&zone_info,
#ifdef CONFIG_TEST
	&tests_info,
	&test_info,
#endif
	NULL
};


/** Initialize command info structure.
 *
 * @param cmd Command info structure.
 *
 */
void cmd_initialize(cmd_info_t *cmd)
{
	spinlock_initialize(&cmd->lock, "cmd");
	link_initialize(&cmd->link);
}

/** Initialize and register commands. */
void cmd_init(void)
{
	int i;

	for (i=0;basic_commands[i]; i++) {
		cmd_initialize(basic_commands[i]);
		if (!cmd_register(basic_commands[i]))
			panic("could not register command %s\n", 
			      basic_commands[i]->name);
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
	link_t *cur;

	spinlock_lock(&cmd_lock);
	
	for (cur = cmd_head.next; cur != &cmd_head; cur = cur->next) {
		cmd_info_t *hlp;
		
		hlp = list_get_instance(cur, cmd_info_t, link);
		spinlock_lock(&hlp->lock);
		
		printf("%s - %s\n", hlp->name, hlp->description);

		spinlock_unlock(&hlp->lock);
	}
	
	spinlock_unlock(&cmd_lock);	

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
	link_t *cur;

	spinlock_lock(&cmd_lock);
	
	for (cur = cmd_head.next; cur != &cmd_head; cur = cur->next) {
		cmd_info_t *hlp;
		
		hlp = list_get_instance(cur, cmd_info_t, link);
		spinlock_lock(&hlp->lock);

		if (strncmp(hlp->name, (const char *) argv->buffer, strlen(hlp->name)) == 0) {
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
	symtab_print_search(argv->buffer);
	
	return 1;
}

/** Call function with zero parameters */
int cmd_call0(cmd_arg_t *argv)
{
	uintptr_t symaddr;
	char *symbol;
	unative_t (*f)(void);
#ifdef ia64
	struct {
		unative_t f;
		unative_t gp;
	}fptr;
#endif

	symaddr = get_symbol_addr(argv->buffer);
	if (!symaddr)
		printf("Symbol %s not found.\n", argv->buffer);
	else if (symaddr == (uintptr_t) -1) {
		symtab_print_search(argv->buffer);
		printf("Duplicate symbol, be more specific.\n");
	} else {
		symbol = get_symtab_entry(symaddr);
		printf("Calling f(): %.*p: %s\n", sizeof(uintptr_t) * 2, symaddr, symbol);
#ifdef ia64
		fptr.f = symaddr;
		fptr.gp = ((unative_t *)cmd_call2)[1];
		f =  (unative_t (*)(void)) &fptr;
#else
		f =  (unative_t (*)(void)) symaddr;
#endif
		printf("Result: %#zx\n", f());
	}
	
	return 1;
}

/** Call function with one parameter */
int cmd_call1(cmd_arg_t *argv)
{
	uintptr_t symaddr;
	char *symbol;
	unative_t (*f)(unative_t,...);
	unative_t arg1 = argv[1].intval;
#ifdef ia64
	struct {
		unative_t f;
		unative_t gp;
	}fptr;
#endif

	symaddr = get_symbol_addr(argv->buffer);
	if (!symaddr)
		printf("Symbol %s not found.\n", argv->buffer);
	else if (symaddr == (uintptr_t) -1) {
		symtab_print_search(argv->buffer);
		printf("Duplicate symbol, be more specific.\n");
	} else {
		symbol = get_symtab_entry(symaddr);

		printf("Calling f(%#zx): %.*p: %s\n", arg1, sizeof(uintptr_t) * 2, symaddr, symbol);
#ifdef ia64
		fptr.f = symaddr;
		fptr.gp = ((unative_t *)cmd_call2)[1];
		f =  (unative_t (*)(unative_t,...)) &fptr;
#else
		f =  (unative_t (*)(unative_t,...)) symaddr;
#endif
		printf("Result: %#zx\n", f(arg1));
	}
	
	return 1;
}

/** Call function with two parameters */
int cmd_call2(cmd_arg_t *argv)
{
	uintptr_t symaddr;
	char *symbol;
	unative_t (*f)(unative_t,unative_t,...);
	unative_t arg1 = argv[1].intval;
	unative_t arg2 = argv[2].intval;
#ifdef ia64
	struct {
		unative_t f;
		unative_t gp;
	}fptr;
#endif

	symaddr = get_symbol_addr(argv->buffer);
	if (!symaddr)
		printf("Symbol %s not found.\n", argv->buffer);
	else if (symaddr == (uintptr_t) -1) {
		symtab_print_search(argv->buffer);
		printf("Duplicate symbol, be more specific.\n");
	} else {
		symbol = get_symtab_entry(symaddr);
		printf("Calling f(0x%zx,0x%zx): %.*p: %s\n", 
		       arg1, arg2, sizeof(uintptr_t) * 2, symaddr, symbol);
#ifdef ia64
		fptr.f = symaddr;
		fptr.gp = ((unative_t *)cmd_call2)[1];
		f =  (unative_t (*)(unative_t,unative_t,...)) &fptr;
#else
		f =  (unative_t (*)(unative_t,unative_t,...)) symaddr;
#endif
		printf("Result: %#zx\n", f(arg1, arg2));
	}
	
	return 1;
}

/** Call function with three parameters */
int cmd_call3(cmd_arg_t *argv)
{
	uintptr_t symaddr;
	char *symbol;
	unative_t (*f)(unative_t,unative_t,unative_t,...);
	unative_t arg1 = argv[1].intval;
	unative_t arg2 = argv[2].intval;
	unative_t arg3 = argv[3].intval;
#ifdef ia64
	struct {
		unative_t f;
		unative_t gp;
	}fptr;
#endif

	symaddr = get_symbol_addr(argv->buffer);
	if (!symaddr)
		printf("Symbol %s not found.\n", argv->buffer);
	else if (symaddr == (uintptr_t) -1) {
		symtab_print_search(argv->buffer);
		printf("Duplicate symbol, be more specific.\n");
	} else {
		symbol = get_symtab_entry(symaddr);
		printf("Calling f(0x%zx,0x%zx, 0x%zx): %.*p: %s\n", 
		       arg1, arg2, arg3, sizeof(uintptr_t) * 2, symaddr, symbol);
#ifdef ia64
		fptr.f = symaddr;
		fptr.gp = ((unative_t *)cmd_call2)[1];
		f =  (unative_t (*)(unative_t,unative_t,unative_t,...)) &fptr;
#else
		f =  (unative_t (*)(unative_t,unative_t,unative_t,...)) symaddr;
#endif
		printf("Result: %#zx\n", f(arg1, arg2, arg3));
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

/** Write 4 byte value to address */
int cmd_set4(cmd_arg_t *argv)
{
	uint32_t *addr ;
	uint32_t arg1 = argv[1].intval;
	bool pointer = false;

	if (((char *)argv->buffer)[0] == '*') {
		addr = (uint32_t *) get_symbol_addr(argv->buffer+1);
		pointer = true;
	} else if (((char *)argv->buffer)[0] >= '0' && 
		   ((char *)argv->buffer)[0] <= '9')
		addr = (uint32_t *)atoi((char *)argv->buffer);
	else
		addr = (uint32_t *)get_symbol_addr(argv->buffer);

	if (!addr)
		printf("Symbol %s not found.\n", argv->buffer);
	else if (addr == (uint32_t *) -1) {
		symtab_print_search(argv->buffer);
		printf("Duplicate symbol, be more specific.\n");
	} else {
		if (pointer)
			addr = (uint32_t *)(*(unative_t *)addr);
		printf("Writing 0x%x -> %.*p\n", arg1, sizeof(uintptr_t) * 2, addr);
		*addr = arg1;
		
	}
	
	return 1;
}

/** Command for listings SLAB caches
 *
 * @param argv Ignores
 *
 * @return Always 1
 */
int cmd_slabs(cmd_arg_t * argv) {
	slab_print_list();
	return 1;
}


/** Command for listings Thread information
 *
 * @param argv Ignores
 *
 * @return Always 1
 */
int cmd_threads(cmd_arg_t * argv) {
	thread_print_list();
	return 1;
}

/** Command for listings Task information
 *
 * @param argv Ignores
 *
 * @return Always 1
 */
int cmd_tasks(cmd_arg_t * argv) {
	task_print_list();
	return 1;
}

/** Command for listings Thread information
 *
 * @param argv Ignores
 *
 * @return Always 1
 */
int cmd_sched(cmd_arg_t * argv) {
	sched_print_list();
	return 1;
}

/** Command for listing memory zones
 *
 * @param argv Ignored
 *
 * return Always 1
 */
int cmd_zones(cmd_arg_t * argv) {
	zone_print_list();
	return 1;
}

/** Command for memory zone details
 *
 * @param argv Integer argument from cmdline expected
 *
 * return Always 1
 */
int cmd_zone(cmd_arg_t * argv) {
	zone_print_one(argv[0].intval);
	return 1;
}

/** Command for printing task ipc details
 *
 * @param argv Integer argument from cmdline expected
 *
 * return Always 1
 */
int cmd_ipc_task(cmd_arg_t * argv) {
	ipc_print_task(argv[0].intval);
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
	printf("Use userspace controls to redraw the screen.\n");
	arch_release_console();
	return 1;
}

#ifdef CONFIG_TEST
/** Command for printing kernel tests list.
 *
 * @param argv Ignored.
 *
 * return Always 1.
 */
int cmd_tests(cmd_arg_t *argv)
{
	test_t *test;
	
	for (test = tests; test->name != NULL; test++)
		printf("%s\t\t%s%s\n", test->name, test->desc, (test->safe ? "" : " (unsafe)"));
	
	printf("*\t\tRun all safe tests\n");
	return 1;
}

static bool run_test(const test_t *test)
{
	printf("%s\t\t%s\n", test->name, test->desc);
	
	/* Update and read thread accounting
	   for benchmarking */
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&TASK->lock);
	uint64_t t0 = task_get_accounting(TASK);
	spinlock_unlock(&TASK->lock);
	interrupts_restore(ipl);
	
	/* Execute the test */
	char * ret = test->entry();
	
	/* Update and read thread accounting */
	ipl = interrupts_disable();
	spinlock_lock(&TASK->lock);
	uint64_t dt = task_get_accounting(TASK) - t0;
	spinlock_unlock(&TASK->lock);
	interrupts_restore(ipl);
	
	printf("Time: %llu cycles\n", dt);
	
	if (ret == NULL) {
		printf("Test passed\n");
		return true;
	}

	printf("%s\n", ret);
	return false;
}

/** Command for returning kernel tests
 *
 * @param argv Argument vector.
 *
 * return Always 1.
 */
int cmd_test(cmd_arg_t *argv)
{
	test_t *test;
	
	if (strcmp(argv->buffer, "*") == 0) {
		for (test = tests; test->name != NULL; test++) {
			if (test->safe) {
				printf("\n");
				if (!run_test(test))
					break;
			}
		}
	} else {
		bool fnd = false;
		
		for (test = tests; test->name != NULL; test++) {
			if (strcmp(test->name, argv->buffer) == 0) {
				fnd = true;
				run_test(test);
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
