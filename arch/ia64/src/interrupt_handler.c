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
 *
 */


#include <panic.h>
#include <print.h>
#include <arch/types.h>
#include <arch/asm.h>
#include <symtab.h>

extern __u64 REG_DUMP;


void general_exception(void);
void general_exception(void)
{
    panic("\nGeneral Exception\n");
}



void break_instruction(void);
void break_instruction(void)
{
    panic("\nBreak Instruction\n");
}


#define cr_dump(r) {__u64 val; get_control_register(r,val); printf("\ncr"#r":%Q",val);}
#define ar_dump(r) {__u64 val; get_aplication_register(r,val); printf("\nar"#r":%Q",val);}

void universal_handler(void);
void universal_handler(void)
{
	__u64 vector,psr,PC;
	__u64 *p;
	int i;
	char *sym;
	
	
	get_shadow_register(16,vector);

	
	p=&REG_DUMP;

	for(i=0;i<128;i+=2) printf("gr%d:%Q\tgr%d:%Q\n",i,p[i],i+1,p[i+1]);


	cr_dump(0);	
	cr_dump(1);	
	cr_dump(2);	
	cr_dump(8);	
	cr_dump(16);	
	cr_dump(17);	
	cr_dump(19);get_control_register(19,PC); if(sym=get_symtab_entry(PC)) printf("(%s)",sym);
	cr_dump(20);get_control_register(20,PC); if(sym=get_symtab_entry(PC)) printf("(%s)",sym);	
	cr_dump(21);	
	cr_dump(22);get_control_register(22,PC); if(sym=get_symtab_entry(PC)) printf("(%s)",sym);	
	cr_dump(23);	
	cr_dump(24);	
	cr_dump(25);	
	cr_dump(64);	
	cr_dump(65);	
	cr_dump(66);	
	cr_dump(67);	
	cr_dump(68);	
	cr_dump(69);	
	cr_dump(70);	
	cr_dump(71);	
	cr_dump(72);	
	cr_dump(73);	
	cr_dump(74);	
	cr_dump(80);	
	cr_dump(81);	
	
	ar_dump(0);	
	ar_dump(1);	
	ar_dump(2);	
	ar_dump(3);	
	ar_dump(4);	
	ar_dump(5);	
	ar_dump(6);	
	ar_dump(7);	
	ar_dump(16);	
	ar_dump(17);	
	ar_dump(18);	
	ar_dump(19);	
	ar_dump(21);	
	ar_dump(24);	
	ar_dump(25);	
	ar_dump(26);	
	ar_dump(27);	
	ar_dump(28);	
	ar_dump(29);	
	ar_dump(30);	
	ar_dump(32);	
	ar_dump(36);	
	ar_dump(40);	
	ar_dump(44);	
	ar_dump(64);	
	ar_dump(65);	
	ar_dump(66);	

	get_psr(psr);

	printf("\nPSR:%Q\n",psr);
	
	panic("\nException:%Q\n",vector);
}


