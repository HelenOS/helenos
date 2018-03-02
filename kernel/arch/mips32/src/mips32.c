/*
 * Copyright (c) 2003-2004 Jakub Jermar
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

/** @addtogroup mips32
 * @{
 */
/** @file
 */

#include <arch.h>
#include <arch/arch.h>
#include <typedefs.h>
#include <errno.h>
#include <interrupt.h>
#include <macros.h>
#include <str.h>
#include <mem.h>
#include <userspace.h>
#include <syscall/syscall.h>
#include <sysinfo/sysinfo.h>
#include <arch/debug.h>
#include <arch/debugger.h>
#include <arch/machine_func.h>

/* Size of the code jumping to the exception handler code
 * - J+NOP
 */
#define EXCEPTION_JUMP_SIZE  8

#define TLB_EXC    ((char *) 0x80000000)
#define NORM_EXC   ((char *) 0x80000180)
#define CACHE_EXC  ((char *) 0x80000100)

static void mips32_pre_mm_init(void);
static void mips32_post_mm_init(void);
static void mips32_post_smp_init(void);

arch_ops_t mips32_ops = {
	.pre_mm_init = mips32_pre_mm_init,
	.post_mm_init = mips32_post_mm_init,
	.post_smp_init = mips32_post_smp_init,
};

arch_ops_t *arch_ops = &mips32_ops;

/* Why the linker moves the variable 64K away in assembler
 * when not in .text section?
 */

/* Stack pointer saved when entering user mode */
uintptr_t supervisor_sp __attribute__ ((section (".text")));

size_t cpu_count = 0;

#if defined(MACHINE_lmalta) || defined(MACHINE_bmalta)
size_t sdram_size = 0;
#endif

/** Performs mips32-specific initialization before main_bsp() is called. */
void mips32_pre_main(void *entry __attribute__((unused)), bootinfo_t *bootinfo)
{
	init.cnt = min3(bootinfo->cnt, TASKMAP_MAX_RECORDS, CONFIG_INIT_TASKS);

	size_t i;
	for (i = 0; i < init.cnt; i++) {
		init.tasks[i].paddr = KA2PA(bootinfo->tasks[i].addr);
		init.tasks[i].size = bootinfo->tasks[i].size;
		str_cpy(init.tasks[i].name, CONFIG_TASK_NAME_BUFLEN,
		    bootinfo->tasks[i].name);
	}

	for (i = 0; i < CPUMAP_MAX_RECORDS; i++) {
		if ((bootinfo->cpumap & (1 << i)) != 0)
			cpu_count++;
	}

#if defined(MACHINE_lmalta) || defined(MACHINE_bmalta)
	sdram_size = bootinfo->sdram_size;
#endif

	/* Initialize machine_ops pointer. */
	machine_ops_init();
}

void mips32_pre_mm_init(void)
{
	/* It is not assumed by default */
	interrupts_disable();

	/* Initialize dispatch table */
	exception_init();

	/* Copy the exception vectors to the right places */
	memcpy(TLB_EXC, (char *) tlb_refill_entry, EXCEPTION_JUMP_SIZE);
	smc_coherence_block(TLB_EXC, EXCEPTION_JUMP_SIZE);
	memcpy(NORM_EXC, (char *) exception_entry, EXCEPTION_JUMP_SIZE);
	smc_coherence_block(NORM_EXC, EXCEPTION_JUMP_SIZE);
	memcpy(CACHE_EXC, (char *) cache_error_entry, EXCEPTION_JUMP_SIZE);
	smc_coherence_block(CACHE_EXC, EXCEPTION_JUMP_SIZE);

	/*
	 * Switch to BEV normal level so that exception vectors point to the
	 * kernel. Clear the error level.
	 */
	cp0_status_write(cp0_status_read() &
	    ~(cp0_status_bev_bootstrap_bit | cp0_status_erl_error_bit));

	/*
	 * Mask all interrupts
	 */
	cp0_mask_all_int();

	debugger_init();
}

void mips32_post_mm_init(void)
{
	interrupt_init();

	machine_init();
	machine_output_init();
}

void mips32_post_smp_init(void)
{
	/* Set platform name. */
	sysinfo_set_item_data("platform", NULL,
	    (void *) machine_get_platform_name(),
	    str_size(machine_get_platform_name()));

	machine_input_init();
}

void calibrate_delay_loop(void)
{
}

void userspace(uspace_arg_t *kernel_uarg)
{
	/* EXL = 1, UM = 1, IE = 1 */
	cp0_status_write(cp0_status_read() | (cp0_status_exl_exception_bit |
	    cp0_status_um_bit | cp0_status_ie_enabled_bit));
	cp0_epc_write((uintptr_t) kernel_uarg->uspace_entry);
	userspace_asm(((uintptr_t) kernel_uarg->uspace_stack +
	    kernel_uarg->uspace_stack_size),
	    (uintptr_t) kernel_uarg->uspace_uarg,
	    (uintptr_t) kernel_uarg->uspace_entry);

	while (1);
}

/** Perform mips32 specific tasks needed before the new task is run. */
void before_task_runs_arch(void)
{
}

/** Perform mips32 specific tasks needed before the new thread is scheduled. */
void before_thread_runs_arch(void)
{
	supervisor_sp =
	    (uintptr_t) &THREAD->kstack[STACK_SIZE];
}

void after_thread_ran_arch(void)
{
}

void arch_reboot(void)
{
	___halt();
	while (1);
}

/** Construct function pointer
 *
 * @param fptr   function pointer structure
 * @param addr   function address
 * @param caller calling function address
 *
 * @return address of the function pointer
 *
 */
void *arch_construct_function(fncptr_t *fptr, void *addr, void *caller)
{
	return addr;
}

void irq_initialize_arch(irq_t *irq)
{
	(void) irq;
}

/** @}
 */
