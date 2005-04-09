/*
 * Copyright (C) 2001-2004 Jakub Jermar
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
#include <panic.h>
#include <arch/cp0.h>
#include <arch/types.h>
#include <arch.h>

void exception(void)
{
	int excno;
	__u32 epc;
	pri_t pri;
	
	pri = cpu_priority_high();
	epc = cp0_epc_read();
	cp0_status_write(cp0_status_read() & ~ cp0_status_exl_exception_bit);

	if (THREAD) {
		THREAD->saved_pri = pri;
		THREAD->saved_epc = epc;
	}
	/* decode exception number and process the exception */
	switch(excno = (cp0_cause_read()>>2)&0x1f) {
		case EXC_Int: interrupt(); break;
		case EXC_TLBL:
		case EXC_TLBS: tlb_invalid(); break;
		default: panic(PANIC "unhandled exception %d\n", excno); break;
	}
	
	if (THREAD) {
		pri = THREAD->saved_pri;
		epc = THREAD->saved_epc;
	}

	cp0_epc_write(epc);
	cp0_status_write(cp0_status_read() | cp0_status_exl_exception_bit);
	cpu_priority_restore(pri);
}
