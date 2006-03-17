/*
 * Copyright (C) 2006 Ondrej Palkovsky
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
#include <console/kconsole.h>
#include <console/cmd.h>
#include <symtab.h>
#include <print.h>
#include <panic.h>
#include <interrupt.h>
#include <arch/asm.h>
#include <arch/cpu.h>
#include <debug.h>
#include <func.h>

typedef struct  {
	__address address;      /**< Breakpoint address */
	int flags;              /**< Flags regarding breakpoint */
	int counter;            /**< How many times the exception occured */
} bpinfo_t;

static bpinfo_t breakpoints[BKPOINTS_MAX];
SPINLOCK_INITIALIZE(bkpoint_lock);

static int cmd_print_breakpoints(cmd_arg_t *argv);
static cmd_info_t bkpts_info = {
	.name = "bkpts",
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
	.description = "addbkpt <&symbol> - new breakpoint.",
	.func = cmd_add_breakpoint,
	.argc = 1,
	.argv = &add_argv
};

static cmd_arg_t addw_argv = {
	.type = ARG_TYPE_INT
};
static cmd_info_t addwatchp_info = {
	.name = "addwatchp",
	.description = "addbwatchp <&symbol> - new write watchpoint.",
	.func = cmd_add_breakpoint,
	.argc = 1,
	.argv = &addw_argv
};


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
			printf("     Count(%d) ", breakpoints[i].counter);
			printf("\n");
		}
	return 1;
}

/** Enable hardware breakpoint
 *
 *
 * @param where Address of HW breakpoint
 * @param flags Type of breakpoint (EXECUTE, WRITE)
 * @return Debug slot on success, -1 - no available HW breakpoint
 */
int breakpoint_add(void * where, int flags)
{
	bpinfo_t *cur = NULL;
	int curidx;
	ipl_t ipl;
	int i;
	__native dr7;

	ASSERT( flags & (BKPOINT_INSTR | BKPOINT_WRITE | BKPOINT_READ_WRITE));

	ipl = interrupts_disable();
	spinlock_lock(&bkpoint_lock);
	
	/* Find free space in slots */
	for (i=0; i<BKPOINTS_MAX; i++)
		if (!breakpoints[i].address) {
			cur = &breakpoints[i];
			curidx = i;
			break;
		}
	if (!cur) {
		/* Too many breakpoints */
		spinlock_unlock(&bkpoint_lock);
		interrupts_restore(ipl);
		return -1;
	}
	cur->address = (__address) where;
	cur->flags = flags;
	cur->counter = 0;

	/* Set breakpoint to debug registers */
	switch (curidx) {
	case 0:
		write_dr0(cur->address);
		break;
	case 1:
		write_dr1(cur->address);
		break;
	case 2:
		write_dr2(cur->address);
		break;
	case 3:
		write_dr3(cur->address);
		break;
	}
	dr7 = read_dr7();
	/* Set type to requested breakpoint & length*/
	dr7 &= ~ (0x3 << (16 + 4*curidx));
	dr7 &= ~ (0x3 << (18 + 4*curidx));
	if ((flags & BKPOINT_INSTR)) {
		printf("Instr breakpoint\n");
		;
	} else {
		if (sizeof(int) == 4)
			dr7 |= 0x3 << (18 + 4*curidx);
		else /* 8 */
			dr7 |= 0x2 << (18 + 4*curidx);
			
		if ((flags & BKPOINT_WRITE))
			dr7 |= 0x1 << (16 + 4*curidx);
		else if ((flags & BKPOINT_READ_WRITE))
			dr7 |= 0x3 << (16 + 4*curidx);
	}

	/* Enable global breakpoint */
	dr7 |= 0x2 << (curidx*2);

	write_dr7(dr7);

	spinlock_unlock(&bkpoint_lock);
	interrupts_restore(ipl);

	return curidx;
}

static void handle_exception(int slot, istate_t *istate)
{
	ASSERT(breakpoints[slot].address);

	/* Handle zero checker */
	if (! (breakpoints[slot].flags & BKPOINT_INSTR)) {
		if ((breakpoints[slot].flags & BKPOINT_CHECK_ZERO)) {
			if (*((__native *) breakpoints[slot].address) != 0)
				return;
			printf("**** Found ZERO on address %P ****\n",
			       slot, breakpoints[slot].address);
		} else {
			printf("Data watchpoint - new data: %P\n",
			       *((__native *) breakpoints[slot].address));
		}
	}
	printf("Reached breakpoint %d:%P(%s)\n", slot, istate->rip,
	       get_symtab_entry(istate->rip));
	printf("***Type 'exit' to exit kconsole.\n");
	atomic_set(&haltstate,1);
	kconsole("debug");
	atomic_set(&haltstate,0);
}

static void debug_exception(int n, istate_t *istate)
{
	__native dr6;
	int i;
	
	/* Set RF to restart the instruction  */
	istate->rflags |= RFLAGS_RF;

	dr6 = read_dr6();
	for (i=0; i < BKPOINTS_MAX; i++) {
		if (dr6 & (1 << i)) {
			dr6 &= ~ (1 << i);
			write_dr6(dr6);
			
			handle_exception(i, istate);
		}
	}
}

void breakpoint_del(int slot)
{
	bpinfo_t *cur;
	ipl_t ipl;
	__native dr7;

	ipl = interrupts_disable();
	spinlock_lock(&bkpoint_lock);

	cur = &breakpoints[slot];
	if (!cur->address) {
		spinlock_unlock(&bkpoint_lock);
		interrupts_restore(ipl);
		return;
	}

	cur->address = NULL;

	/* Disable breakpoint in DR7 */
	dr7 = read_dr7();
	dr7 &= ~(0x2 << (slot*2));
	write_dr7(dr7);

	spinlock_unlock(&bkpoint_lock);
	interrupts_restore(ipl);
}

/** Remove breakpoint from table */
int cmd_del_breakpoint(cmd_arg_t *argv)
{
	if (argv->intval < 0 || argv->intval > BKPOINTS_MAX) {
		printf("Invalid breakpoint number.\n");
		return 0;
	}
	breakpoint_del(argv->intval);
	return 1;
}

/** Add new breakpoint to table */
static int cmd_add_breakpoint(cmd_arg_t *argv)
{
	int flags;

	if (argv == &add_argv) {
		flags = BKPOINT_INSTR;
	} else { /* addwatchp */
		flags = BKPOINT_WRITE;
	}
	printf("Adding breakpoint on address: %p\n", argv->intval);
	if (breakpoint_add((void *)argv->intval, flags))
		printf("Add breakpoint failed.\n");
	
	return 1;
}

/** Initialize debugger */
void debugger_init()
{
	int i;

	for (i=0; i<BKPOINTS_MAX; i++)
		breakpoints[i].address = NULL;
	
	cmd_initialize(&bkpts_info);
	if (!cmd_register(&bkpts_info))
		panic("could not register command %s\n", bkpts_info.name);

	cmd_initialize(&delbkpt_info);
	if (!cmd_register(&delbkpt_info))
		panic("could not register command %s\n", delbkpt_info.name);

	cmd_initialize(&addbkpt_info);
	if (!cmd_register(&addbkpt_info))
		panic("could not register command %s\n", addbkpt_info.name);

	cmd_initialize(&addwatchp_info);
	if (!cmd_register(&addwatchp_info))
		panic("could not register command %s\n", addwatchp_info.name);
	
	exc_register(VECTOR_DEBUG, "debugger",
		     debug_exception);
}
