/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_abs32le
 * @{
 */
/** @file
 */

#include <arch.h>
#include <typedefs.h>
#include <arch/interrupt.h>
#include <arch/asm.h>

#include <halt.h>
#include <config.h>
#include <console/console.h>
#include <errno.h>
#include <context.h>
#include <fpu_context.h>
#include <interrupt.h>
#include <ddi/irq.h>
#include <syscall/copy.h>
#include <syscall/syscall.h>

static void abs32le_post_mm_init(void);

arch_ops_t abs32le_ops = {
	.post_mm_init = abs32le_post_mm_init,
};

arch_ops_t *arch_ops = &abs32le_ops;

char memcpy_from_uspace_failover_address;
char memcpy_to_uspace_failover_address;

void abs32le_post_mm_init(void)
{
	if (config.cpu_active == 1) {
		/* Initialize IRQ routing */
		irq_init(0, 0);

		/* Merge all memory zones to 1 big zone */
		zone_merge_all();
	}
}

void calibrate_delay_loop(void)
{
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

void arch_reboot(void)
{
}

void irq_initialize_arch(irq_t *irq)
{
	(void) irq;
}

void istate_decode(istate_t *istate)
{
	(void) istate;
}

int context_save_arch(context_t *ctx)
{
	return EOK;
}

void context_restore_arch(context_t *ctx)
{
	while (true)
		;
}

void fpu_init(void)
{
}

void fpu_context_save(fpu_context_t *ctx)
{
}

void fpu_context_restore(fpu_context_t *ctx)
{
}

uintptr_t memcpy_from_uspace(void *dst, uspace_addr_t uspace_src, size_t size)
{
	return 0;
}

uintptr_t memcpy_to_uspace(uspace_addr_t uspace_dst, const void *src, size_t size)
{
	return 0;
}

void early_putuchar(char32_t ch)
{
}

/** @}
 */
