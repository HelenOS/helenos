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

/** @addtogroup sparc64interrupt
 * @{
 */
/** @file
 */

#include <arch/interrupt.h>
#include <arch/trap/interrupt.h>
#include <interrupt.h>
#include <arch/drivers/fhc.h>
#include <typedefs.h>
#include <arch/types.h>
#include <debug.h>
#include <ipc/sysipc.h>
#include <arch/asm.h>
#include <arch/barrier.h>

#include <genarch/kbd/z8530.h>

/** Register Interrupt Level Handler.
 *
 * @param n Interrupt Level (1 - 15).
 * @param name Short descriptive string.
 * @param f Handler.
 */
void interrupt_register(int n, const char *name, iroutine f)
{
	ASSERT(n >= IVT_FIRST && n <= IVT_ITEMS);
	
	exc_register(n - 1, name, f);
}

/* Reregister irq to be IPC-ready */
void irq_ipc_bind_arch(unative_t irq)
{
	panic("not implemented\n");
	/* TODO */
}

void interrupt(int n, istate_t *istate)
{
	uint64_t intrcv;
	uint64_t data0;

	intrcv = asi_u64_read(ASI_INTR_RECEIVE, 0);
	data0 = asi_u64_read(ASI_UDB_INTR_R, ASI_UDB_INTR_R_DATA_0);

	switch (data0) {
#ifdef CONFIG_Z8530
	case Z8530_INTRCV_DATA0:
		/*
		 * So far, we know we got this interrupt through the FHC.
		 * Since we don't have enough information about the FHC and
		 * because the interrupt looks like level sensitive,
		 * we cannot handle it by scheduling one of the level
		 * interrupt traps. Call the interrupt handler directly.
		 */
		fhc_uart_reset();
		z8530_interrupt();
		break;
#endif
	}

	membar();
	asi_u64_write(ASI_INTR_RECEIVE, 0, 0);
}

/** @}
 */
