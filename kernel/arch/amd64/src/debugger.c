/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup amd64debug
 * @{
 */
/** @file
 */

#include <arch/debugger.h>
#include <console/kconsole.h>
#include <console/cmd.h>
#include <print.h>
#include <panic.h>
#include <interrupt.h>
#include <arch/asm.h>
#include <arch/cpu.h>
#include <debug.h>
#include <func.h>
#include <smp/ipi.h>
#include <symtab.h>

#ifdef __64_BITS__
	#define getip(x)  ((x)->rip)
#endif

#ifdef __32_BITS__
	#define getip(x)  ((x)->eip)
#endif

typedef struct  {
	uintptr_t address;   /**< Breakpoint address */
	unsigned int flags;  /**< Flags regarding breakpoint */
	size_t counter;      /**< How many times the exception occured */
} bpinfo_t;

static bpinfo_t breakpoints[BKPOINTS_MAX];
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
	.description = "Add breakpoint.",
	.func = cmd_add_breakpoint,
	.argc = 1,
	.argv = &add_argv
};

static cmd_arg_t addw_argv = {
	.type = ARG_TYPE_INT
};

static cmd_info_t addwatchp_info = {
	.name = "addwatchp",
	.description = "Add write watchpoint.",
	.func = cmd_add_breakpoint,
	.argc = 1,
	.argv = &addw_argv
};

#endif /* CONFIG_KCONSOLE */

/** Setup DR register according to table
 *
 */
static void setup_dr(int curidx)
{
	ASSERT(curidx >= 0);
	
	bpinfo_t *cur = &breakpoints[curidx];
	unsigned int flags = breakpoints[curidx].flags;
	
	/* Disable breakpoint in DR7 */
	unative_t dr7 = read_dr7();
	dr7 &= ~(0x2 << (curidx * 2));
	
	/* Setup DR register */
	if (cur->address) {
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
		
		/* Set type to requested breakpoint & length*/
		dr7 &= ~(0x3 << (16 + 4 * curidx));
		dr7 &= ~(0x3 << (18 + 4 * curidx));
		
		if (!(flags & BKPOINT_INSTR)) {
#ifdef __32_BITS__
			dr7 |= ((unative_t) 0x3) << (18 + 4 * curidx);
#endif
			
#ifdef __64_BITS__
			dr7 |= ((unative_t) 0x2) << (18 + 4 * curidx);
#endif
			
			if ((flags & BKPOINT_WRITE))
				dr7 |= ((unative_t) 0x1) << (16 + 4 * curidx);
			else if ((flags & BKPOINT_READ_WRITE))
				dr7 |= ((unative_t) 0x3) << (16 + 4 * curidx);
		}
		
		/* Enable global breakpoint */
		dr7 |= 0x2 << (curidx * 2);
		
		write_dr7(dr7);
	}
}

/** Enable hardware breakpoint
 *
 * @param where Address of HW breakpoint
 * @param flags Type of breakpoint (EXECUTE, WRITE)
 *
 * @return Debug slot on success, -1 - no available HW breakpoint
 *
 */
int breakpoint_add(const void *where, const unsigned int flags, int curidx)
{
	ASSERT(flags & (BKPOINT_INSTR | BKPOINT_WRITE | BKPOINT_READ_WRITE));
	
	irq_spinlock_lock(&bkpoint_lock, true);
	
	if (curidx == -1) {
		/* Find free space in slots */
		unsigned int i;
		for (i = 0; i < BKPOINTS_MAX; i++) {
			if (!breakpoints[i].address) {
				curidx = i;
				break;
			}
		}
		
		if (curidx == -1) {
			/* Too many breakpoints */
			irq_spinlock_unlock(&bkpoint_lock, true);
			return -1;
		}
	}
	
	bpinfo_t *cur = &breakpoints[curidx];
	
	cur->address = (uintptr_t) where;
	cur->flags = flags;
	cur->counter = 0;
	
	setup_dr(curidx);
	
	irq_spinlock_unlock(&bkpoint_lock, true);
	
	/* Send IPI */
//	ipi_broadcast(VECTOR_DEBUG_IPI);
	
	return curidx;
}

static void handle_exception(int slot, istate_t *istate)
{
	ASSERT(slot >= 0);
	ASSERT(breakpoints[slot].address);
	
	/* Handle zero checker */
	if (!(breakpoints[slot].flags & BKPOINT_INSTR)) {
		if ((breakpoints[slot].flags & BKPOINT_CHECK_ZERO)) {
			if (*((unative_t *) breakpoints[slot].address) != 0)
				return;
			
			printf("*** Found ZERO on address %lx (slot %d) ***\n",
			    breakpoints[slot].address, slot);
		} else {
			printf("Data watchpoint - new data: %lx\n",
			    *((unative_t *) breakpoints[slot].address));
		}
	}
	
	printf("Reached breakpoint %d:%lx (%s)\n", slot, getip(istate),
	    symtab_fmt_name_lookup(getip(istate)));
	
#ifdef CONFIG_KCONSOLE
	atomic_set(&haltstate, 1);
	kconsole("debug", "Debug console ready.\n", false);
	atomic_set(&haltstate, 0);
#endif
}

void breakpoint_del(int slot)
{
	ASSERT(slot >= 0);
	
	irq_spinlock_lock(&bkpoint_lock, true);
	
	bpinfo_t *cur = &breakpoints[slot];
	if (!cur->address) {
		irq_spinlock_unlock(&bkpoint_lock, true);
		return;
	}
	
	cur->address = NULL;
	
	setup_dr(slot);
	
	irq_spinlock_unlock(&bkpoint_lock, true);
//	ipi_broadcast(VECTOR_DEBUG_IPI);
}

static void debug_exception(unsigned int n __attribute__((unused)), istate_t *istate)
{
	/* Set RF to restart the instruction  */
#ifdef __64_BITS__
	istate->rflags |= RFLAGS_RF;
#endif
	
#ifdef __32_BITS__
	istate->eflags |= EFLAGS_RF;
#endif
	
	unative_t dr6 = read_dr6();
	
	unsigned int i;
	for (i = 0; i < BKPOINTS_MAX; i++) {
		if (dr6 & (1 << i)) {
			dr6 &= ~ (1 << i);
			write_dr6(dr6);
			
			handle_exception(i, istate);
		}
	}
}

#ifdef CONFIG_SMP
static void debug_ipi(unsigned int n __attribute__((unused)),
    istate_t *istate __attribute__((unused)))
{
	irq_spinlock_lock(&bkpoint_lock, false);
	
	unsigned int i;
	for (i = 0; i < BKPOINTS_MAX; i++)
		setup_dr(i);
	
	irq_spinlock_unlock(&bkpoint_lock, false);
}
#endif /* CONFIG_SMP */

/** Initialize debugger
 *
 */
void debugger_init()
{
	unsigned int i;
	for (i = 0; i < BKPOINTS_MAX; i++)
		breakpoints[i].address = NULL;
	
#ifdef CONFIG_KCONSOLE
	cmd_initialize(&bkpts_info);
	if (!cmd_register(&bkpts_info))
		printf("Cannot register command %s\n", bkpts_info.name);
	
	cmd_initialize(&delbkpt_info);
	if (!cmd_register(&delbkpt_info))
		printf("Cannot register command %s\n", delbkpt_info.name);
	
	cmd_initialize(&addbkpt_info);
	if (!cmd_register(&addbkpt_info))
		printf("Cannot register command %s\n", addbkpt_info.name);
	
	cmd_initialize(&addwatchp_info);
	if (!cmd_register(&addwatchp_info))
		printf("Cannot register command %s\n", addwatchp_info.name);
#endif /* CONFIG_KCONSOLE */
	
	exc_register(VECTOR_DEBUG, "debugger", true,
	    debug_exception);
	
#ifdef CONFIG_SMP
	exc_register(VECTOR_DEBUG_IPI, "debugger_smp", true,
	    debug_ipi);
#endif /* CONFIG_SMP */
}

#ifdef CONFIG_KCONSOLE
/** Print table of active breakpoints
 *
 */
int cmd_print_breakpoints(cmd_arg_t *argv __attribute__((unused)))
{
#ifdef __32_BITS__
	printf("#  Count Address    In symbol\n");
	printf("-- ----- ---------- ---------\n");
#endif
	
#ifdef __64_BITS__
	printf("#  Count Address            In symbol\n");
	printf("-- ----- ------------------ ---------\n");
#endif
	
	unsigned int i;
	for (i = 0; i < BKPOINTS_MAX; i++) {
		if (breakpoints[i].address) {
			const char *symbol = symtab_fmt_name_lookup(
			    breakpoints[i].address);
			
#ifdef __32_BITS__
			printf("%-2u %-5" PRIs " %p %s\n", i,
			    breakpoints[i].counter, breakpoints[i].address,
			    symbol);
#endif
			
#ifdef __64_BITS__
			printf("%-2u %-5" PRIs " %p %s\n", i,
			    breakpoints[i].counter, breakpoints[i].address,
			    symbol);
#endif
		}
	}
	
	return 1;
}

/** Remove breakpoint from table
 *
 */
int cmd_del_breakpoint(cmd_arg_t *argv)
{
	unative_t bpno = argv->intval;
	if (bpno > BKPOINTS_MAX) {
		printf("Invalid breakpoint number.\n");
		return 0;
	}
	
	breakpoint_del(argv->intval);
	return 1;
}

/** Add new breakpoint to table
 *
 */
static int cmd_add_breakpoint(cmd_arg_t *argv)
{
	unsigned int flags;
	if (argv == &add_argv)
		flags = BKPOINT_INSTR;
	else
		flags = BKPOINT_WRITE;
	
	printf("Adding breakpoint on address: %p\n", argv->intval);
	
	int id = breakpoint_add((void *)argv->intval, flags, -1);
	if (id < 0)
		printf("Add breakpoint failed.\n");
	else
		printf("Added breakpoint %d.\n", id);
	
	return 1;
}
#endif /* CONFIG_KCONSOLE */

/** @}
 */
