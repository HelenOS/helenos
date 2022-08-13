/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#include <smp/ipi.h>
#include <arch/barrier.h>
#include <assert.h>
#include <cpu.h>
#include <arch.h>
#include <arch/cpu.h>
#include <arch/asm.h>
#include <config.h>
#include <mm/tlb.h>
#include <arch/interrupt.h>
#include <arch/trap/interrupt.h>
#include <barrier.h>
#include <preemption.h>
#include <time/delay.h>
#include <panic.h>

/** Set the contents of the outgoing interrupt vector data.
 *
 * The first data item (data 0) will be set to the value of func, the
 * rest of the vector will contain zeros.
 *
 * This is a helper function used from within the cross_call function.
 *
 * @param func value the first data item of the vector will be set to
 */
static inline void set_intr_w_data(void (*func)(void))
{
#if defined (US)
	asi_u64_write(ASI_INTR_W, ASI_UDB_INTR_W_DATA_0, (uintptr_t) func);
	asi_u64_write(ASI_INTR_W, ASI_UDB_INTR_W_DATA_1, 0);
	asi_u64_write(ASI_INTR_W, ASI_UDB_INTR_W_DATA_2, 0);
#elif defined (US3)
	asi_u64_write(ASI_INTR_W, VA_INTR_W_DATA_0, (uintptr_t)	func);
	asi_u64_write(ASI_INTR_W, VA_INTR_W_DATA_1, 0);
	asi_u64_write(ASI_INTR_W, VA_INTR_W_DATA_2, 0);
	asi_u64_write(ASI_INTR_W, VA_INTR_W_DATA_3, 0);
	asi_u64_write(ASI_INTR_W, VA_INTR_W_DATA_4, 0);
	asi_u64_write(ASI_INTR_W, VA_INTR_W_DATA_5, 0);
	asi_u64_write(ASI_INTR_W, VA_INTR_W_DATA_6, 0);
	asi_u64_write(ASI_INTR_W, VA_INTR_W_DATA_7, 0);
#endif
}

/** Invoke function on another processor.
 *
 * Currently, only functions without arguments are supported.
 * Supporting more arguments in the future should be no big deal.
 *
 * Interrupts must be disabled prior to this call.
 *
 * @param mid MID of the target processor.
 * @param func Function to be invoked.
 */
static void cross_call(int mid, void (*func)(void))
{
	uint64_t status;
	bool done;

	/*
	 * This function might enable interrupts for a while.
	 * In order to prevent migration to another processor,
	 * we explicitly disable preemption.
	 */

	preemption_disable();

	status = asi_u64_read(ASI_INTR_DISPATCH_STATUS, 0);
	if (status & INTR_DISPATCH_STATUS_BUSY)
		panic("Interrupt Dispatch Status busy bit set\n");

	assert(!(pstate_read() & PSTATE_IE_BIT));

	do {
		set_intr_w_data(func);
		asi_u64_write(ASI_INTR_W,
		    (mid << INTR_VEC_DISPATCH_MID_SHIFT) |
		    VA_INTR_W_DISPATCH, 0);

		membar();

		do {
			status = asi_u64_read(ASI_INTR_DISPATCH_STATUS, 0);
		} while (status & INTR_DISPATCH_STATUS_BUSY);

		done = !(status & INTR_DISPATCH_STATUS_NACK);
		if (!done) {
			/*
			 * Prevent deadlock.
			 */
			(void) interrupts_enable();
			delay(20 + (tick_read() & 0xff));
			(void) interrupts_disable();
		}
	} while (!done);

	preemption_enable();
}

/*
 * Deliver IPI to all processors except the current one.
 *
 * The sparc64 architecture does not support any group addressing
 * which is found, for instance, on ia32 and amd64. Therefore we
 * need to simulate the broadcast by sending the message to
 * all target processors step by step.
 *
 * We assume that interrupts are disabled.
 *
 * @param ipi IPI number.
 */
void ipi_broadcast_arch(int ipi)
{
	unsigned int i;

	void (*func)(void);

	switch (ipi) {
	case IPI_TLB_SHOOTDOWN:
		func = tlb_shootdown_ipi_recv;
		break;
	default:
		panic("Unknown IPI (%d).\n", ipi);
		break;
	}

	/*
	 * As long as we don't support hot-plugging
	 * or hot-unplugging of CPUs, we can walk
	 * the cpus array and read processor's MID
	 * without locking.
	 */

	for (i = 0; i < config.cpu_active; i++) {
		if (&cpus[i] == CPU)
			continue;		/* skip the current CPU */

		cross_call(cpus[i].arch.mid, func);
	}
}

/** @}
 */
