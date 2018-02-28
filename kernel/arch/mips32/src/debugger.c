/*
 * Copyright (c) 2005 Ondrej Palkovsky
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

/** @addtogroup mips32debug
 * @{
 */
/** @file
 */

#include <arch/debugger.h>
#include <arch/barrier.h>
#include <console/kconsole.h>
#include <console/cmd.h>
#include <print.h>
#include <log.h>
#include <panic.h>
#include <arch.h>
#include <arch/cp0.h>
#include <halt.h>
#include <symtab.h>

bpinfo_t breakpoints[BKPOINTS_MAX];
IRQ_SPINLOCK_STATIC_INITIALIZE(bkpoint_lock);

#ifdef CONFIG_KCONSOLE

static int cmd_print_breakpoints(cmd_arg_t *);
static int cmd_del_breakpoint(cmd_arg_t *);
static int cmd_add_breakpoint(cmd_arg_t *);

static cmd_info_t bkpts_info = {
	.name = "bkpts",
	.description = "Print breakpoint table.",
	.func = cmd_print_breakpoints,
	.argc = 0,
};

static cmd_arg_t del_argv = {
	.type = ARG_TYPE_INT
};

static cmd_info_t delbkpt_info = {
	.name = "delbkpt",
	.description = "Delete breakpoint.",
	.func = cmd_del_breakpoint,
	.argc = 1,
	.argv = &del_argv
};

static cmd_arg_t add_argv = {
	.type = ARG_TYPE_INT
};

static cmd_info_t addbkpt_info = {
	.name = "addbkpt",
	.description = "Add bkpoint (break on j/branch insts unsupported).",
	.func = cmd_add_breakpoint,
	.argc = 1,
	.argv = &add_argv
};

static cmd_arg_t adde_argv[] = {
	{ .type = ARG_TYPE_INT },
	{ .type = ARG_TYPE_INT }
};
static cmd_info_t addbkpte_info = {
	.name = "addbkpte",
	.description = "Add bkpoint with a trigger function.",
	.func = cmd_add_breakpoint,
	.argc = 2,
	.argv = adde_argv
};
#endif

static struct {
	uint32_t andmask;
	uint32_t value;
} jmpinstr[] = {
	{0xf3ff0000, 0x41000000},  /* BCzF */
	{0xf3ff0000, 0x41020000},  /* BCzFL */
	{0xf3ff0000, 0x41010000},  /* BCzT */
	{0xf3ff0000, 0x41030000},  /* BCzTL */
	{0xfc000000, 0x10000000},  /* BEQ */
	{0xfc000000, 0x50000000},  /* BEQL */
	{0xfc1f0000, 0x04010000},  /* BEQL */
	{0xfc1f0000, 0x04110000},  /* BGEZAL */
	{0xfc1f0000, 0x04130000},  /* BGEZALL */
	{0xfc1f0000, 0x04030000},  /* BGEZL */
	{0xfc1f0000, 0x1c000000},  /* BGTZ */
	{0xfc1f0000, 0x5c000000},  /* BGTZL */
	{0xfc1f0000, 0x18000000},  /* BLEZ */
	{0xfc1f0000, 0x58000000},  /* BLEZL */
	{0xfc1f0000, 0x04000000},  /* BLTZ */
	{0xfc1f0000, 0x04100000},  /* BLTZAL */
	{0xfc1f0000, 0x04120000},  /* BLTZALL */
	{0xfc1f0000, 0x04020000},  /* BLTZL */
	{0xfc000000, 0x14000000},  /* BNE */
	{0xfc000000, 0x54000000},  /* BNEL */
	{0xfc000000, 0x08000000},  /* J */
	{0xfc000000, 0x0c000000},  /* JAL */
	{0xfc1f07ff, 0x00000009},  /* JALR */
	{0, 0}                     /* end of table */
};

/** Test, if the given instruction is a jump or branch instruction
 *
 * @param instr Instruction code
 *
 * @return true if it is jump instruction, false otherwise
 *
 */
bool is_jump(sysarg_t instr)
{
	unsigned int i;

	for (i = 0; jmpinstr[i].andmask; i++) {
		if ((instr & jmpinstr[i].andmask) == jmpinstr[i].value)
			return true;
	}

	return false;
}

#ifdef CONFIG_KCONSOLE

/** Add new breakpoint to table
 *
 */
int cmd_add_breakpoint(cmd_arg_t *argv)
{
	if (argv->intval & 0x3) {
		printf("Not aligned instruction, forgot to use &symbol?\n");
		return 1;
	}

	irq_spinlock_lock(&bkpoint_lock, true);

	/* Check, that the breakpoints do not conflict */
	unsigned int i;
	for (i = 0; i < BKPOINTS_MAX; i++) {
		if (breakpoints[i].address == (uintptr_t) argv->intval) {
			printf("Duplicate breakpoint %d.\n", i);
			irq_spinlock_unlock(&bkpoint_lock, true);
			return 0;
		} else if ((breakpoints[i].address == (uintptr_t) argv->intval +
		    sizeof(sysarg_t)) || (breakpoints[i].address ==
		    (uintptr_t) argv->intval - sizeof(sysarg_t))) {
			printf("Adjacent breakpoints not supported, conflict "
			    "with %d.\n", i);
			irq_spinlock_unlock(&bkpoint_lock, true);
			return 0;
		}

	}

	bpinfo_t *cur = NULL;

	for (i = 0; i < BKPOINTS_MAX; i++) {
		if (!breakpoints[i].address) {
			cur = &breakpoints[i];
			break;
		}
	}

	if (!cur) {
		printf("Too many breakpoints.\n");
		irq_spinlock_unlock(&bkpoint_lock, true);
		return 0;
	}

	printf("Adding breakpoint on address %p\n", (void *) argv->intval);

	cur->address = (uintptr_t) argv->intval;
	cur->instruction = ((sysarg_t *) cur->address)[0];
	cur->nextinstruction = ((sysarg_t *) cur->address)[1];
	if (argv == &add_argv) {
		cur->flags = 0;
	} else {  /* We are add extended */
		cur->flags = BKPOINT_FUNCCALL;
		cur->bkfunc = (void (*)(void *, istate_t *)) argv[1].intval;
	}

	if (is_jump(cur->instruction))
		cur->flags |= BKPOINT_ONESHOT;

	cur->counter = 0;

	/* Set breakpoint */
	*((sysarg_t *) cur->address) = 0x0d;
	smc_coherence(cur->address);

	irq_spinlock_unlock(&bkpoint_lock, true);

	return 1;
}

/** Remove breakpoint from table
 *
 */
int cmd_del_breakpoint(cmd_arg_t *argv)
{
	if (argv->intval > BKPOINTS_MAX) {
		printf("Invalid breakpoint number.\n");
		return 0;
	}

	irq_spinlock_lock(&bkpoint_lock, true);

	bpinfo_t *cur = &breakpoints[argv->intval];
	if (!cur->address) {
		printf("Breakpoint does not exist.\n");
		irq_spinlock_unlock(&bkpoint_lock, true);
		return 0;
	}

	if ((cur->flags & BKPOINT_INPROG) && (cur->flags & BKPOINT_ONESHOT)) {
		printf("Cannot remove one-shot breakpoint in-progress\n");
		irq_spinlock_unlock(&bkpoint_lock, true);
		return 0;
	}

	((uint32_t *) cur->address)[0] = cur->instruction;
	smc_coherence(((uint32_t *) cur->address)[0]);
	((uint32_t *) cur->address)[1] = cur->nextinstruction;
	smc_coherence(((uint32_t *) cur->address)[1]);

	cur->address = (uintptr_t) NULL;

	irq_spinlock_unlock(&bkpoint_lock, true);
	return 1;
}

/** Print table of active breakpoints
 *
 */
int cmd_print_breakpoints(cmd_arg_t *argv)
{
	unsigned int i;

	printf("[nr] [count] [address ] [inprog] [oneshot] [funccall] [in symbol\n");

	for (i = 0; i < BKPOINTS_MAX; i++) {
		if (breakpoints[i].address) {
			const char *symbol = symtab_fmt_name_lookup(
			    breakpoints[i].address);

			printf("%-4u %7zu %p %-8s %-9s %-10s %s\n", i,
			    breakpoints[i].counter, (void *) breakpoints[i].address,
			    ((breakpoints[i].flags & BKPOINT_INPROG) ? "true" :
			    "false"), ((breakpoints[i].flags & BKPOINT_ONESHOT)
			    ? "true" : "false"), ((breakpoints[i].flags &
			    BKPOINT_FUNCCALL) ? "true" : "false"), symbol);
		}
	}

	return 1;
}

#endif /* CONFIG_KCONSOLE */

/** Initialize debugger
 *
 */
void debugger_init(void)
{
	unsigned int i;

	for (i = 0; i < BKPOINTS_MAX; i++)
		breakpoints[i].address = (uintptr_t) NULL;

#ifdef CONFIG_KCONSOLE
	cmd_initialize(&bkpts_info);
	if (!cmd_register(&bkpts_info))
		log(LF_OTHER, LVL_WARN, "Cannot register command %s",
		    bkpts_info.name);

	cmd_initialize(&delbkpt_info);
	if (!cmd_register(&delbkpt_info))
		log(LF_OTHER, LVL_WARN, "Cannot register command %s",
		    delbkpt_info.name);

	cmd_initialize(&addbkpt_info);
	if (!cmd_register(&addbkpt_info))
		log(LF_OTHER, LVL_WARN, "Cannot register command %s",
		    addbkpt_info.name);

	cmd_initialize(&addbkpte_info);
	if (!cmd_register(&addbkpte_info))
		log(LF_OTHER, LVL_WARN, "Cannot register command %s",
		    addbkpte_info.name);
#endif /* CONFIG_KCONSOLE */
}

/** Handle breakpoint
 *
 * Find breakpoint in breakpoint table.
 * If found, call kconsole, set break on next instruction and reexecute.
 * If we are on "next instruction", set it back on the first and reexecute.
 * If breakpoint not found in breakpoint table, call kconsole and start
 * next instruction.
 *
 */
void debugger_bpoint(istate_t *istate)
{
	/* test branch delay slot */
	if (cp0_cause_read() & 0x80000000)
		panic("Breakpoint in branch delay slot not supported.");

	irq_spinlock_lock(&bkpoint_lock, false);

	bpinfo_t *cur = NULL;
	uintptr_t fireaddr = istate->epc;
	unsigned int i;

	for (i = 0; i < BKPOINTS_MAX; i++) {
		/* Normal breakpoint */
		if ((fireaddr == breakpoints[i].address) &&
		    (!(breakpoints[i].flags & BKPOINT_REINST))) {
			cur = &breakpoints[i];
			break;
		}

		/* Reinst only breakpoint */
		if ((breakpoints[i].flags & BKPOINT_REINST) &&
		    (fireaddr == breakpoints[i].address + sizeof(sysarg_t))) {
			cur = &breakpoints[i];
			break;
		}
	}

	if (cur) {
		if (cur->flags & BKPOINT_REINST) {
			/* Set breakpoint on first instruction */
			((uint32_t *) cur->address)[0] = 0x0d;
			smc_coherence(((uint32_t *)cur->address)[0]);

			/* Return back the second */
			((uint32_t *) cur->address)[1] = cur->nextinstruction;
			smc_coherence(((uint32_t *) cur->address)[1]);

			cur->flags &= ~BKPOINT_REINST;
			irq_spinlock_unlock(&bkpoint_lock, false);
			return;
		}

		if (cur->flags & BKPOINT_INPROG)
			printf("Warning: breakpoint recursion\n");

		if (!(cur->flags & BKPOINT_FUNCCALL)) {
			printf("***Breakpoint %u: %p in %s.\n", i,
			    (void *) fireaddr,
			    symtab_fmt_name_lookup(fireaddr));
		}

		/* Return first instruction back */
		((uint32_t *)cur->address)[0] = cur->instruction;
		smc_coherence(cur->address);

		if (! (cur->flags & BKPOINT_ONESHOT)) {
			/* Set Breakpoint on next instruction */
			((uint32_t *)cur->address)[1] = 0x0d;
			cur->flags |= BKPOINT_REINST;
		}
		cur->flags |= BKPOINT_INPROG;
	} else {
		printf("***Breakpoint %d: %p in %s.\n", i,
		    (void *) fireaddr,
		    symtab_fmt_name_lookup(fireaddr));

		/* Move on to next instruction */
		istate->epc += 4;
	}

	if (cur)
		cur->counter++;

	if (cur && (cur->flags & BKPOINT_FUNCCALL)) {
		/* Allow zero bkfunc, just for counting */
		if (cur->bkfunc)
			cur->bkfunc(cur, istate);
	} else {
#ifdef CONFIG_KCONSOLE
		/*
		 * This disables all other processors - we are not SMP,
		 * actually this gets us to cpu_halt, if scheduler() is run
		 * - we generally do not want scheduler to be run from debug,
		 *   so this is a good idea
		 */
		atomic_set(&haltstate, 1);
		irq_spinlock_unlock(&bkpoint_lock, false);

		kconsole("debug", "Debug console ready.\n", false);

		irq_spinlock_lock(&bkpoint_lock, false);
		atomic_set(&haltstate, 0);
#endif
	}

	if ((cur) && (cur->address == fireaddr)
	    && ((cur->flags & BKPOINT_INPROG))) {
		/* Remove one-shot breakpoint */
		if ((cur->flags & BKPOINT_ONESHOT))
			cur->address = (uintptr_t) NULL;

		/* Remove in-progress flag */
		cur->flags &= ~BKPOINT_INPROG;
	}

	irq_spinlock_unlock(&bkpoint_lock, false);
}

/** @}
 */
