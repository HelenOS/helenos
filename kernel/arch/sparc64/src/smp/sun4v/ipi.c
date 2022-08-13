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
#include <cpu.h>
#include <config.h>
#include <interrupt.h>
#include <arch/asm.h>
#include <arch/cpu.h>
#include <arch/sun4v/hypercall.h>
#include <arch/sun4v/ipi.h>

#define IPI_MESSAGE_SIZE	8

static uint64_t data[MAX_NUM_STRANDS][IPI_MESSAGE_SIZE]
    __attribute__((aligned(64)));

static uint16_t ipi_cpu_list[MAX_NUM_STRANDS][MAX_NUM_STRANDS];

/**
 * Sends an inter-processor interrupt to all virtual processors whose IDs are
 * listed in the list.
 *
 * @param func		address of the function to be invoked on the recipient
 * @param cpu_list	list of CPU IDs (16-bit identifiers) of the recipients
 * @param list_size	number of recipients (number of valid cpu_list entries)
 *
 * @return		error code returned by the CPU_MONDO_SEND hypercall
 */
uint64_t ipi_brodcast_to(void (*func)(void), uint16_t cpu_list[MAX_NUM_STRANDS],
    uint64_t list_size)
{

	data[CPU->arch.id][0] = (uint64_t) func;

	unsigned int i;
	for (i = 0; i < list_size; i++) {
		ipi_cpu_list[CPU->arch.id][i] = cpu_list[i];
	}

	return __hypercall_fast3(CPU_MONDO_SEND, list_size,
	    KA2PA(ipi_cpu_list[CPU->arch.id]), KA2PA(data[CPU->arch.id]));
}

/**
 * Send an inter-processor interrupt to a particular CPU.
 *
 * @param func		address of the function to be invoked on the recipient
 * @param cpu_is	CPU ID (16-bit identifier) of the recipient
 *
 * @return		error code returned by the CPU_MONDO_SEND hypercall
 */
uint64_t ipi_unicast_to(void (*func)(void), uint16_t cpu_id)
{
	ipi_cpu_list[CPU->arch.id][0] = cpu_id;
	return ipi_brodcast_to(func, ipi_cpu_list[CPU->arch.id], 1);
}

/*
 * Deliver IPI to all processors except the current one.
 *
 * We assume that interrupts are disabled.
 *
 * @param ipi IPI number.
 */
void ipi_broadcast_arch(int ipi)
{
	void (*func)(void);

	switch (ipi) {
	case IPI_TLB_SHOOTDOWN:
		func = tlb_shootdown_ipi_recv;
		break;
	default:
		panic("Unknown IPI (%d).\n", ipi);
		break;
	}

	unsigned int i;
	unsigned idx = 0;
	for (i = 0; i < config.cpu_active; i++) {
		if (&cpus[i] == CPU) {
			continue;
		}

		ipi_cpu_list[CPU->id][idx] = (uint16_t) cpus[i].id;
		idx++;
	}

	ipi_brodcast_to(func, ipi_cpu_list[CPU->arch.id], idx);
}

/** @}
 */
