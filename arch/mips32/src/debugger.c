/*
 * Copyright (C) 2005 Ondrej Palkovsky
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

#include <arch/debugger.h>
#include <memstr.h>
#include <console/kconsole.h>
#include <console/cmd.h>
#include <symtab.h>
#include <print.h>
#include <panic.h>
#include <arch.h>
#include <arch/cp0.h>
#include <func.h>

bpinfo_t breakpoints[BKPOINTS_MAX];
spinlock_t bkpoint_lock;

static int cmd_print_breakpoints(cmd_arg_t *argv);
static cmd_info_t pbkpt_info = {
	.name = "pbkpt",
	.description = "Print breakpoint table.",
	.func = cmd_print_breakpoints,
	.argc = 0,
};

static int cmd_del_breakpoint(cmd_arg_t *argv);
static cmd_arg_t del_argv = {
	.type = ARG_TYPE_INT
};
static cmd_info_t delbkpt_info = {
	.name = "delbkpt",
	.description = "delbkpt <number> - Delete breakpoint.",
	.func = cmd_del_breakpoint,
	.argc = 1,
	.argv = &del_argv
};

static int cmd_add_breakpoint(cmd_arg_t *argv);
static cmd_arg_t add_argv = {
	.type = ARG_TYPE_INT
};
static cmd_info_t addbkpt_info = {
	.name = "addbkpt",
	.description = "addbkpt <&symbol> - new bkpoint. Break on J/Branch insts unsupported.",
	.func = cmd_add_breakpoint,
	.argc = 1,
	.argv = &add_argv
};

/** Add new breakpoint to table */
int cmd_add_breakpoint(cmd_arg_t *argv)
{
	bpinfo_t *cur = NULL;
	ipl_t ipl;
	int i;

	if (argv->intval & 0x3) {
		printf("Not aligned instruction, forgot to use &symbol?\n");
		return 1;
	}
	ipl = interrupts_disable();
	spinlock_lock(&bkpoint_lock);

	/* Check, that the breakpoints do not conflict */
	for (i=0; i<BKPOINTS_MAX; i++) {
		if (breakpoints[i].address == (__address)argv->intval) {
			printf("Duplicate breakpoint %d.\n", i);
			spinlock_unlock(&bkpoints_lock);
			return 0;
		} else if (breakpoints[i].address == (__address)argv->intval + sizeof(__native) || \
			   breakpoints[i].address == (__address)argv->intval - sizeof(__native)) {
			printf("Adjacent breakpoints not supported, conflict with %d.\n", i);
			spinlock_unlock(&bkpoints_lock);
			return 0;
		}
			
	}

	for (i=0; i<BKPOINTS_MAX; i++)
		if (!breakpoints[i].address) {
			cur = &breakpoints[i];
			break;
		}
	if (!cur) {
		printf("Too many breakpoints.\n");
		spinlock_unlock(&bkpoint_lock);
		interrupts_restore(ipl);
		return 0;
	}
	cur->address = (__address) argv->intval;
	printf("Adding breakpoint on address: %p\n", argv->intval);
	cur->instruction = ((__native *)cur->address)[0];
	cur->nextinstruction = ((__native *)cur->address)[1];
	cur->executing = false;

	/* Set breakpoint */
	*((__native *)cur->address) = 0x0d;

	spinlock_unlock(&bkpoint_lock);
	interrupts_restore(ipl);

	return 1;
}

/** Remove breakpoint from table */
int cmd_del_breakpoint(cmd_arg_t *argv)
{
	bpinfo_t *cur;
	ipl_t ipl;

	if (argv->intval < 0 || argv->intval > BKPOINTS_MAX) {
		printf("Invalid breakpoint number.\n");
		return 0;
	}
	ipl = interrupts_disable();
	spinlock_lock(&bkpoint_lock);

	cur = &breakpoints[argv->intval];
	if (!cur->address) {
		printf("Breakpoint does not exist.\n");
		spinlock_unlock(&bkpoint_lock);
		interrupts_restore(ipl);
		return 0;
	}
	
	((__u32 *)cur->address)[0] = cur->instruction;
	((__u32 *)cur->address)[1] = cur->nextinstruction;

	cur->address = NULL;

	spinlock_unlock(&bkpoint_lock);
	interrupts_restore(ipl);
	return 1;
}

/** Print table of active breakpoints */
int cmd_print_breakpoints(cmd_arg_t *argv)
{
	int i;
	char *symbol;

	printf("Breakpoint table.\n");
	for (i=0; i < BKPOINTS_MAX; i++)
		if (breakpoints[i].address) {
			symbol = get_symtab_entry(breakpoints[i].address);
			printf("%d. 0x%p in %s\n",i,
			       breakpoints[i].address, symbol);
		}
	return 1;
}

/** Initialize debugger */
void debugger_init()
{
	int i;

	for (i=0; i<BKPOINTS_MAX; i++)
		breakpoints[i].address = NULL;
	spinlock_initialize(&bkpoint_lock, "breakpoint_lock");
	
	cmd_initialize(&pbkpt_info);
	if (!cmd_register(&pbkpt_info))
		panic("could not register command %s\n", pbkpt_info.name);

	cmd_initialize(&delbkpt_info);
	if (!cmd_register(&delbkpt_info))
		panic("could not register command %s\n", delbkpt_info.name);

	cmd_initialize(&addbkpt_info);
	if (!cmd_register(&addbkpt_info))
		panic("could not register command %s\n", addbkpt_info.name);
}

/** Handle breakpoint
 *
 * Find breakpoint in breakpoint table. 
 * If found, call kconsole, set break on next instruction and reexecute.
 * If we are on "next instruction", set it back on the first and reexecute.
 * If breakpoint not found in breakpoint table, call kconsole and start
 * next instruction.
 */
void debugger_bpoint(struct exception_regdump *pstate)
{
	char *symbol;
	bpinfo_t *cur = NULL;
	int i;

	symbol = get_symtab_entry(pstate->epc);
	

	/* test branch delay slot */
	if (cp0_cause_read() & 0x80000000)
		panic("Breakpoint in branch delay slot not supported.\n");

	spinlock_lock(&bkpoint_lock);
	for (i=0; i<BKPOINTS_MAX; i++) {
		if (pstate->epc == breakpoints[i].address || \
		    (pstate->epc == breakpoints[i].address+sizeof(__native) &&\
		     breakpoints[i].executing))
			cur = &breakpoints[i];
		break;

	}
	if (cur) {
		if ((cur->executing && pstate->epc==cur->address) ||
		    (!cur->executing && pstate->epc==cur->address+sizeof(__native)))
			panic("Weird breakpoint state.\n");
		if (!cur->executing) {
			printf("***Breakpoint %d: %p in %s.\n", i, 
			       pstate->epc,symbol);
			/* Return first instruction back */
			((__u32 *)cur->address)[0] = cur->instruction;
			/* Set Breakpoint on second */
			((__u32 *)cur->address)[1] = 0x0d;
			cur->executing = true;
		} else {
			/* Set breakpoint on first instruction */
			((__u32 *)cur->address)[0] = 0x0d;
			/* Return back the second */
			((__u32 *)cur->address)[1] = cur->nextinstruction;
			cur->executing = false;
			spinlock_unlock(&bkpoint_lock);
			return;
		}
	} else {
		printf("***Breakpoint %p in %s.\n", pstate->epc, symbol);
		/* Move on to next instruction */
		pstate->epc += 4;
	}
	spinlock_unlock(&bkpoint_lock);

			
	printf("***Type 'exit' to exit kconsole.\n");
	/* Umm..we should rather set some 'debugstate' here */
	haltstate = 1;
	kconsole("debug");
	haltstate = 0;
}
