/*
 * Copyright (C) 2003-2004 Jakub Jermar
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

#include <arch/exception.h>
#include <arch/interrupt.h>
#include <panic.h>
#include <arch/cp0.h>
#include <arch/types.h>
#include <arch.h>
#include <debug.h>

void exception(struct exception_regdump *pstate)
{
	int excno;
	__u32 epc_shift = 0;

	ASSERT(CPU != NULL);

	/*
	 * NOTE ON OPERATION ORDERING
	 *
	 * On entry, cpu_priority_high() must be called before 
	 * exception bit is cleared.
	 */

	cpu_priority_high();
	cp0_status_write(cp0_status_read() & ~ (cp0_status_exl_exception_bit |
						cp0_status_um_bit));
	/* Save pstate so that the threads can access it */
	if (THREAD)
		THREAD->pstate = pstate;

	/* decode exception number and process the exception */
	switch (excno = (cp0_cause_read() >> 2) & 0x1f) {
		case EXC_Int:
			interrupt();
			break;
		case EXC_TLBL:
		case EXC_TLBS:
			tlb_invalid(pstate);
			break;
		case EXC_Mod:
			panic("unhandled TLB Modification Exception\n");
			break;
		case EXC_AdEL:
			panic("unhandled Address Error Exception - load or instruction fetch\n");
			break;
 	 	case EXC_AdES:
			panic("unhandled Address Error Exception - store\n");
			break;
 	 	case EXC_IBE:
			panic("unhandled Bus Error Exception - fetch instruction\n");
			break;
 	 	case EXC_DBE:
			panic("unhandled Bus Error Exception - data reference: load or store\n");
			break;
		case EXC_Bp:
			/* it is necessary to not re-execute BREAK instruction after returning from Exception handler
			   (see page 138 in R4000 Manual for more information) */
			epc_shift = 4;
			break;
		case EXC_RI:
			panic("unhandled Reserved Instruction Exception\n");
			break;
 	 	case EXC_CpU:
#ifdef FPU_LAZY     
			scheduler_fpu_lazy_request();
#else
			panic("unhandled Coprocessor Unusable Exception\n");
#endif
			break;
 	 	case EXC_Ov:
			panic("unhandled Arithmetic Overflow Exception\n");
			break;
 	 	case EXC_Tr:
			panic("unhandled Trap Exception\n");
			break;
 	 	case EXC_VCEI:
			panic("unhandled Virtual Coherency Exception - instruction\n");
			break;
 	 	case EXC_FPE:
			panic("unhandled Floating-Point Exception\n");
			break;
 	 	case EXC_WATCH:
			panic("unhandled reference to WatchHi/WatchLo address\n");
			break;
 	 	case EXC_VCED:
			panic("unhandled Virtual Coherency Exception - data\n");
			break;
		default:
			panic("unhandled exception %d\n", excno);
	}
	
	pstate->epc += epc_shift;
	/* Probable not needed, but just for sure that nobody 
	 * will continue accessing it */
	if (THREAD)
		THREAD->pstate = NULL;
}
