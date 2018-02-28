/*
 * Copyright (c) 2010 Martin Decky
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

/** @addtogroup abs32le
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
	while (true);
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

errno_t memcpy_from_uspace(void *dst, const void *uspace_src, size_t size)
{
	return EOK;
}

errno_t memcpy_to_uspace(void *uspace_dst, const void *src, size_t size)
{
	return EOK;
}

void early_putchar(wchar_t ch)
{
}

/** @}
 */
