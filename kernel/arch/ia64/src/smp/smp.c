/*
 * Copyright (c) 2005 Jakub Vana
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

/** @addtogroup ia64
 * @{
 */
/** @file
 */

#include <arch.h>
#include <arch/ski/ski.h>
#include <arch/drivers/it.h>
#include <arch/interrupt.h>
#include <arch/barrier.h>
#include <arch/asm.h>
#include <arch/register.h>
#include <arch/types.h>
#include <arch/context.h>
#include <arch/stack.h>
#include <arch/mm/page.h>
#include <mm/as.h>
#include <config.h>
#include <userspace.h>
#include <console/console.h>
#include <proc/uarg.h>
#include <syscall/syscall.h>
#include <ddi/irq.h>
#include <ddi/device.h>
#include <arch/drivers/ega.h>
#include <arch/bootinfo.h>
#include <genarch/kbd/i8042.h>
#include <genarch/kbd/ns16550.h>
#include <smp/smp.h>
#include <smp/ipi.h>
#include <arch/atomic.h>
#include <panic.h>
#include <print.h>






#ifdef CONFIG_SMP


extern char cpu_by_id_eid_list[256][256];


static void sapic_init(void)
{
	bootinfo->sapic=(unative_t *)(PA2KA((unative_t)(bootinfo->sapic))|FW_OFFSET);
}



static void ipi_broadcast_arch_all(int ipi )
{
	int id,eid;
	int myid,myeid;
	
	myid=ia64_get_cpu_id();
	myeid=ia64_get_cpu_eid();

	
	for(id=0;id<256;id++)
		for(eid=0;eid<256;eid++)
			if((id!=myid) || (eid!=myeid))
				ipi_send_ipi(id,eid,ipi);
}

void ipi_broadcast_arch(int ipi )
{
	ipi_broadcast_arch_all(ipi);
}


void smp_init(void)
{
	sapic_init();
	ipi_broadcast_arch_all(bootinfo->wakeup_intno);	
	volatile long long brk;
        for(brk=0;brk<100LL*1024LL*1024LL;brk++); //wait a while before CPUs starts

	config.cpu_count=0;
	int id,eid;
	
	for(id=0;id<256;id++)
		for(eid=0;eid<256;eid++)
		        if(cpu_by_id_eid_list[id][eid]==1){
		    		config.cpu_count++;
		    		cpu_by_id_eid_list[id][eid]=2;

			}
}


void kmp(void *arg __attribute__((unused)))
{
	int id,eid;
	int myid,myeid;
	
	myid=ia64_get_cpu_id();
	myeid=ia64_get_cpu_eid();

	for(id=0;id<256;id++)
		for(eid=0;eid<256;eid++)
		        if((id!=myid) || (eid!=myeid))
		        	if(cpu_by_id_eid_list[id][eid]!=0){
		    			if(cpu_by_id_eid_list[id][eid]==1){
		    		
			    			//config.cpu_count++;
				    		//cpu_by_id_eid_list[id][eid]=2;
				    		printf("Found Late CPU ID:%d EDI:%d Not added to system!!!\n",id,eid);
				    		continue;
			    			}
					cpu_by_id_eid_list[id][eid]=3;
					/*
					 * There may be just one AP being initialized at
					 * the time. After it comes completely up, it is
					 * supposed to wake us up.
					 */
					if (waitq_sleep_timeout(&ap_completion_wq, 1000000,
					    SYNCH_FLAGS_NONE) == ESYNCH_TIMEOUT) {
						printf("%s: waiting for cpu ID:%d EID:%d"
						    "timed out\n", __FUNCTION__, 
						    id, eid);
					    }    
		
				}
}
#endif


/*This is just a hack for linking with assembler - may be removed in future*/
#ifndef CONFIG_SMP
void main_ap(void);
void main_ap(void)
{
	while(1);
}

#endif

/** @}
 */

