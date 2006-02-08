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

/*
 * TLB management.
 */

#include <mm/tlb.h>
#include <arch/mm/tlb.h>


/** Invalidate all TLB entries. */
void tlb_invalidate_all(void)
{
	/* TODO */
}

/** Invalidate entries belonging to an address space.
 *
 * @param asid Address space identifier.
 */
void tlb_invalidate_asid(asid_t asid)
{
	/* TODO */
}



void tlb_fill_data(__address va,asid_t asid,vhpt_entry_t entry)
{
	region_register rr;


	if(!(entry.not_present.p)) return;

	rr.word=rr_read(VA_REGION(va));

	if(rr.map.rid==ASID2RID(asid,VA_REGION(va)))
	{
		asm
		(
			"srlz.i;;\n"
			"srlz.d;;\n"
			"mov r8=psr;;\n"
			"and r9=r8,%0;;\n"   				/*(~PSR_IC_MASK)*/
			"mov psr.l=r9;;\n"
			"srlz.d;;\n"
			"srlz.i;;\n"
			"mov cr20=%1\n"        		/*va*/		/*cr20 == IFA*/ 
			"mov cr21=%2;;\n"					/*entry.word[1]*/ /*cr21=ITIR*/
			"itc.d %3;;\n"						/*entry.word[0]*/
			"mov psr.l=r8;;\n"
			"srlz.d;;\n"
			:
			:"r"(~PSR_IC_MASK),"r"(va),"r"(entry.word[1]),"r"(entry.word[0])
			:"r8","r9"
		);
	}
	else
	{
		region_register rr0;
		rr0=rr;
		rr0.map.rid=ASID2RID(asid,VA_REGION(va));
		rr_write(VA_REGION(va),rr0.word);
		asm
		(
			"mov r8=psr;;\n"
			"and r9=r8,%0;;\n"   				/*(~PSR_IC_MASK)*/
			"mov psr.l=r9;;\n"
			"srlz.d;;\n"
			"mov cr20=%1\n"        		/*va*/		/*cr20 == IFA*/ 
			"mov cr21=%2;;\n"					/*entry.word[1]*/ /*cr21=ITIR*/
			"itc.d %3;;\n"						/*entry.word[0]*/
			"mov psr.l=r8;;\n"
			"srlz.d;;\n"
			:
			:"r"(~PSR_IC_MASK),"r"(va),"r"(entry.word[1]),"r"(entry.word[0])
			:"r8","r9"
		);
		rr_write(VA_REGION(va),rr.word);
	}


}

void tlb_fill_code(__address va,asid_t asid,vhpt_entry_t entry)
{
	region_register rr;


	if(!(entry.not_present.p)) return;

	rr.word=rr_read(VA_REGION(va));

	if(rr.map.rid==ASID2RID(asid,VA_REGION(va)))
	{
		asm
		(
			"srlz.i;;\n"
			"srlz.d;;\n"
			"mov r8=psr;;\n"
			"and r9=r8,%0;;\n"   				/*(~PSR_IC_MASK)*/
			"mov psr.l=r9;;\n"
			"srlz.d;;\n"
			"srlz.i;;\n"
			"mov cr20=%1\n"        		/*va*/		/*cr20 == IFA*/ 
			"mov cr21=%2;;\n"					/*entry.word[1]*/ /*cr21=ITIR*/
			"itc.i %3;;\n"						/*entry.word[0]*/
			"mov psr.l=r8;;\n"
			"srlz.d;;\n"
			:
			:"r"(~PSR_IC_MASK),"r"(va),"r"(entry.word[1]),"r"(entry.word[0])
			:"r8","r9"
		);
	}
	else
	{
		region_register rr0;
		rr0=rr;
		rr0.map.rid=ASID2RID(asid,VA_REGION(va));
		rr_write(VA_REGION(va),rr0.word);
		asm
		(
			"mov r8=psr;;\n"
			"and r9=r8,%0;;\n"   				/*(~PSR_IC_MASK)*/
			"mov psr.l=r9;;\n"
			"srlz.d;;\n"
			"mov cr20=%1\n"        		/*va*/		/*cr20 == IFA*/ 
			"mov cr21=%2;;\n"					/*entry.word[1]*/ /*cr21=ITIR*/
			"itc.i %3;;\n"						/*entry.word[0]*/
			"mov psr.l=r8;;\n"
			"srlz.d;;\n"
			:
			:"r"(~PSR_IC_MASK),"r"(va),"r"(entry.word[1]),"r"(entry.word[0])
			:"r8","r9"
		);
		rr_write(VA_REGION(va),rr.word);
	}


}


