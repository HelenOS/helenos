/*
 * Copyright (C) 2005 Jakub Vana
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

#include <arch/interrupt.h>
#include <print.h>
#include <debug.h>
#include <panic.h>
#include <arch/i8259.h>
#include <func.h>
#include <cpu.h>
#include <arch/asm.h>
#include <mm/tlb.h>

#include <test.h>
#include <arch.h>
#include <arch/smp/atomic.h>
#include <proc/thread.h>

static void e(void *data)
{
	int i;
	while(1) 
	{
		double e,d,le,f;
		le=-1;
		e=0;
		f=1;
		for(i=0,d=1;e!=le;d*=f,f+=1,i++) 
		{
			le=e;
			e=e+1/d;
			if (i>20000000) 
			{
//				printf("tid%d: e LOOPING\n", THREAD->tid);
				putchar('!');
				i = 0;
			}
			
		}
    
		if((int)(100000000*e)==271828182) printf("tid%d: e OK\n", THREAD->tid);
		else panic("tid%d: e FAILED (100000000*e=%d)\n", THREAD->tid, (int) 100000000*e);
	}
}



void test(void)
{
	thread_t *t;
	int i;

	for (i=0; i<4; i++) {  
		t = thread_create(e, NULL, TASK, 0);
		thread_ready(t);
	}
	
	while(1);

}
