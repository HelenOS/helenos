/*
 * Copyright (c) 2006 Jakub Jermar
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
