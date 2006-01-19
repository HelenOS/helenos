/*
 * Copyright (C) 2006 Jakub Jermar
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

#include <arch/mm/page.h>
#include <arch/types.h>
#include <genarch/mm/page_ht.h>
#include <mm/page.h>
#include <config.h>
#include <panic.h>

__u64 thash(__u64 va);
__u64 thash(__u64 va)
{
	__u64 ret;
	asm
	(
		"thash %0=%1;;"
		:"=r"(ret)
		:"r" (va)
	);
	
	return ret;
}

__u64 ttag(__u64 va);
__u64 ttag(__u64 va)
{
	__u64 ret;
	asm
	(
		"ttag %0=%1;;"
		:"=r"(ret)
		:"r" (va)
	);
	
	return ret;
}


static void set_VHPT_environment(void)
{
	return;

	/*
	TODO:
	*/
	
	int i;
	
	/* First set up REGION REGISTER 0 */
	
	region_register rr;
	rr.map.ve=0;                  /*Disable Walker*/
	rr.map.ps=PAGE_WIDTH;
	rr.map.rid=REGION_RID_MAIN;
	
	asm
	(
		"mov rr[r0]=%0;;"
		:
		:"r"(rr.word)
	);
		
	/* And Invalidate the rest of REGION REGISTERS */
	
	for(i=1;i<REGION_REGISTERS;i++)
	{
		rr.map.rid=REGION_RID_FIRST_INVALID+i-1;
		asm
		(
			"mov r8=%1;;"
			"mov rr[r8]=%0;;"
			:
			:"r"(rr.word),"r"(i)
			:"r8"
		);
	};

	PTA_register pta;
	pta.map.ve=0;                   /*Disable Walker*/
	pta.map.vf=1;                   /*Large entry format*/
	pta.map.size=VHPT_WIDTH;
	pta.map.base=VHPT_BASE;
	
	
	/*Write PTA*/
	asm
	(
		"mov cr8=%0;;"
		:
		:"r"(pta.word)
	);	
	
}	


void page_arch_init(void)
{
	page_operations = &page_ht_operations;
	set_VHPT_environment();
}
