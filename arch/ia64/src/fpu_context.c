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

#include <fpu_context.h>
#include <print.h>

void fpu_context_save(fpu_context_t *fctx){
		return;
		asm volatile(
			"stf.spill [%2]=f2,0x80\n"
			"stf.spill [%3]=f3,0x80\n"
			"stf.spill [%4]=f4,0x80\n"
			"stf.spill [%5]=f5,0x80\n"
			"stf.spill [%6]=f6,0x80\n"
			"stf.spill [%7]=f7,0x80\n;;"

			"stf.spill [%0]=f8,0x80\n"
			"stf.spill [%1]=f9,0x80\n"
			"stf.spill [%2]=f10,0x80\n"
			"stf.spill [%3]=f11,0x80\n"
			"stf.spill [%4]=f12,0x80\n"
			"stf.spill [%5]=f13,0x80\n"
			"stf.spill [%6]=f14,0x80\n"
			"stf.spill [%7]=f15,0x80\n;;"

			"stf.spill [%0]=f16,0x80\n"
			"stf.spill [%1]=f17,0x80\n"
			"stf.spill [%2]=f18,0x80\n"
			"stf.spill [%3]=f19,0x80\n"
			"stf.spill [%4]=f20,0x80\n"
			"stf.spill [%5]=f21,0x80\n"
			"stf.spill [%6]=f22,0x80\n"
			"stf.spill [%7]=f23,0x80\n;;"

			"stf.spill [%0]=f24,0x80\n"
			"stf.spill [%1]=f25,0x80\n"
			"stf.spill [%2]=f26,0x80\n"
			"stf.spill [%3]=f27,0x80\n"
			"stf.spill [%4]=f28,0x80\n"
			"stf.spill [%5]=f29,0x80\n"
			"stf.spill [%6]=f30,0x80\n"
			"stf.spill [%7]=f31,0x80\n;;"


			"stf.spill [%0]=f32,0x80\n"
			"stf.spill [%1]=f33,0x80\n"
			"stf.spill [%2]=f34,0x80\n"
			"stf.spill [%3]=f35,0x80\n"
			"stf.spill [%4]=f36,0x80\n"
			"stf.spill [%5]=f37,0x80\n"
			"stf.spill [%6]=f38,0x80\n"
			"stf.spill [%7]=f39,0x80\n;;"

			"stf.spill [%0]=f40,0x80\n"
			"stf.spill [%1]=f41,0x80\n"
			"stf.spill [%2]=f42,0x80\n"
			"stf.spill [%3]=f43,0x80\n"
			"stf.spill [%4]=f44,0x80\n"
			"stf.spill [%5]=f45,0x80\n"
			"stf.spill [%6]=f46,0x80\n"
			"stf.spill [%7]=f47,0x80\n;;"

			"stf.spill [%0]=f48,0x80\n"
			"stf.spill [%1]=f49,0x80\n"
			"stf.spill [%2]=f50,0x80\n"
			"stf.spill [%3]=f51,0x80\n"
			"stf.spill [%4]=f52,0x80\n"
			"stf.spill [%5]=f53,0x80\n"
			"stf.spill [%6]=f54,0x80\n"
			"stf.spill [%7]=f55,0x80\n;;"

			"stf.spill [%0]=f56,0x80\n"
			"stf.spill [%1]=f57,0x80\n"
			"stf.spill [%2]=f58,0x80\n"
			"stf.spill [%3]=f59,0x80\n"
			"stf.spill [%4]=f60,0x80\n"
			"stf.spill [%5]=f61,0x80\n"
			"stf.spill [%6]=f62,0x80\n"
			"stf.spill [%7]=f63,0x80\n;;"

			"stf.spill [%0]=f64,0x80\n"
			"stf.spill [%1]=f65,0x80\n"
			"stf.spill [%2]=f66,0x80\n"
			"stf.spill [%3]=f67,0x80\n"
			"stf.spill [%4]=f68,0x80\n"
			"stf.spill [%5]=f69,0x80\n"
			"stf.spill [%6]=f70,0x80\n"
			"stf.spill [%7]=f71,0x80\n;;"

			"stf.spill [%0]=f72,0x80\n"
			"stf.spill [%1]=f73,0x80\n"
			"stf.spill [%2]=f74,0x80\n"
			"stf.spill [%3]=f75,0x80\n"
			"stf.spill [%4]=f76,0x80\n"
			"stf.spill [%5]=f77,0x80\n"
			"stf.spill [%6]=f78,0x80\n"
			"stf.spill [%7]=f79,0x80\n;;"

			"stf.spill [%0]=f80,0x80\n"
			"stf.spill [%1]=f81,0x80\n"
			"stf.spill [%2]=f82,0x80\n"
			"stf.spill [%3]=f83,0x80\n"
			"stf.spill [%4]=f84,0x80\n"
			"stf.spill [%5]=f85,0x80\n"
			"stf.spill [%6]=f86,0x80\n"
			"stf.spill [%7]=f87,0x80\n;;"

			"stf.spill [%0]=f88,0x80\n"
			"stf.spill [%1]=f89,0x80\n"
			"stf.spill [%2]=f90,0x80\n"
			"stf.spill [%3]=f91,0x80\n"
			"stf.spill [%4]=f92,0x80\n"
			"stf.spill [%5]=f93,0x80\n"
			"stf.spill [%6]=f94,0x80\n"
			"stf.spill [%7]=f95,0x80\n;;"


			"stf.spill [%0]=f96,0x80\n"
			"stf.spill [%1]=f97,0x80\n"
			"stf.spill [%2]=f98,0x80\n"
			"stf.spill [%3]=f99,0x80\n"
			"stf.spill [%4]=f100,0x80\n"
			"stf.spill [%5]=f101,0x80\n"
			"stf.spill [%6]=f102,0x80\n"
			"stf.spill [%7]=f103,0x80\n;;"

			"stf.spill [%0]=f104,0x80\n"
			"stf.spill [%1]=f105,0x80\n"
			"stf.spill [%2]=f106,0x80\n"
			"stf.spill [%3]=f107,0x80\n"
			"stf.spill [%4]=f108,0x80\n"
			"stf.spill [%5]=f109,0x80\n"
			"stf.spill [%6]=f110,0x80\n"
			"stf.spill [%7]=f111,0x80\n;;"

			"stf.spill [%0]=f112,0x80\n"
			"stf.spill [%1]=f113,0x80\n"
			"stf.spill [%2]=f114,0x80\n"
			"stf.spill [%3]=f115,0x80\n"
			"stf.spill [%4]=f116,0x80\n"
			"stf.spill [%5]=f117,0x80\n"
			"stf.spill [%6]=f118,0x80\n"
			"stf.spill [%7]=f119,0x80\n;;"

			"stf.spill [%0]=f120,0x80\n"
			"stf.spill [%1]=f121,0x80\n"
			"stf.spill [%2]=f122,0x80\n"
			"stf.spill [%3]=f123,0x80\n"
			"stf.spill [%4]=f124,0x80\n"
			"stf.spill [%5]=f125,0x80\n"
			"stf.spill [%6]=f126,0x80\n"
			"stf.spill [%7]=f127,0x80\n;;"


			:
			:"r" (&((fctx->fr)[0])),"r" (&((fctx->fr)[1])),"r" (&((fctx->fr)[2])),"r" (&((fctx->fr)[3])),
			 "r" (&((fctx->fr)[4])),"r" (&((fctx->fr)[5])),"r" (&((fctx->fr)[6])),"r" (&((fctx->fr)[7]))
		); 
	
}


void fpu_context_restore(fpu_context_t *fctx)
{
		return;
		asm volatile(
			"ldf.fill f2=[%2],0x80\n"
			"ldf.fill f3=[%3],0x80\n"
			"ldf.fill f4=[%4],0x80\n"
			"ldf.fill f5=[%5],0x80\n"
			"ldf.fill f6=[%6],0x80\n"
			"ldf.fill f7=[%7],0x80\n;;"

			"ldf.fill f8=[%0],0x80\n"
			"ldf.fill f9=[%1],0x80\n"
			"ldf.fill f10=[%2],0x80\n"
			"ldf.fill f11=[%3],0x80\n"
			"ldf.fill f12=[%4],0x80\n"
			"ldf.fill f13=[%5],0x80\n"
			"ldf.fill f14=[%6],0x80\n"
			"ldf.fill f15=[%7],0x80\n;;"

			"ldf.fill f16=[%0],0x80\n"
			"ldf.fill f17=[%1],0x80\n"
			"ldf.fill f18=[%2],0x80\n"
			"ldf.fill f19=[%3],0x80\n"
			"ldf.fill f20=[%4],0x80\n"
			"ldf.fill f21=[%5],0x80\n"
			"ldf.fill f22=[%6],0x80\n"
			"ldf.fill f23=[%7],0x80\n;;"

			"ldf.fill f24=[%0],0x80\n"
			"ldf.fill f25=[%1],0x80\n"
			"ldf.fill f26=[%2],0x80\n"
			"ldf.fill f27=[%3],0x80\n"
			"ldf.fill f28=[%4],0x80\n"
			"ldf.fill f29=[%5],0x80\n"
			"ldf.fill f30=[%6],0x80\n"
			"ldf.fill f31=[%7],0x80\n;;"


			"ldf.fill f32=[%0],0x80\n"
			"ldf.fill f33=[%1],0x80\n"
			"ldf.fill f34=[%2],0x80\n"
			"ldf.fill f35=[%3],0x80\n"
			"ldf.fill f36=[%4],0x80\n"
			"ldf.fill f37=[%5],0x80\n"
			"ldf.fill f38=[%6],0x80\n"
			"ldf.fill f39=[%7],0x80\n;;"

			"ldf.fill f40=[%0],0x80\n"
			"ldf.fill f41=[%1],0x80\n"
			"ldf.fill f42=[%2],0x80\n"
			"ldf.fill f43=[%3],0x80\n"
			"ldf.fill f44=[%4],0x80\n"
			"ldf.fill f45=[%5],0x80\n"
			"ldf.fill f46=[%6],0x80\n"
			"ldf.fill f47=[%7],0x80\n;;"

			"ldf.fill f48=[%0],0x80\n"
			"ldf.fill f49=[%1],0x80\n"
			"ldf.fill f50=[%2],0x80\n"
			"ldf.fill f51=[%3],0x80\n"
			"ldf.fill f52=[%4],0x80\n"
			"ldf.fill f53=[%5],0x80\n"
			"ldf.fill f54=[%6],0x80\n"
			"ldf.fill f55=[%7],0x80\n;;"

			"ldf.fill f56=[%0],0x80\n"
			"ldf.fill f57=[%1],0x80\n"
			"ldf.fill f58=[%2],0x80\n"
			"ldf.fill f59=[%3],0x80\n"
			"ldf.fill f60=[%4],0x80\n"
			"ldf.fill f61=[%5],0x80\n"
			"ldf.fill f62=[%6],0x80\n"
			"ldf.fill f63=[%7],0x80\n;;"

			"ldf.fill f64=[%0],0x80\n"
			"ldf.fill f65=[%1],0x80\n"
			"ldf.fill f66=[%2],0x80\n"
			"ldf.fill f67=[%3],0x80\n"
			"ldf.fill f68=[%4],0x80\n"
			"ldf.fill f69=[%5],0x80\n"
			"ldf.fill f70=[%6],0x80\n"
			"ldf.fill f71=[%7],0x80\n;;"

			"ldf.fill f72=[%0],0x80\n"
			"ldf.fill f73=[%1],0x80\n"
			"ldf.fill f74=[%2],0x80\n"
			"ldf.fill f75=[%3],0x80\n"
			"ldf.fill f76=[%4],0x80\n"
			"ldf.fill f77=[%5],0x80\n"
			"ldf.fill f78=[%6],0x80\n"
			"ldf.fill f79=[%7],0x80\n;;"

			"ldf.fill f80=[%0],0x80\n"
			"ldf.fill f81=[%1],0x80\n"
			"ldf.fill f82=[%2],0x80\n"
			"ldf.fill f83=[%3],0x80\n"
			"ldf.fill f84=[%4],0x80\n"
			"ldf.fill f85=[%5],0x80\n"
			"ldf.fill f86=[%6],0x80\n"
			"ldf.fill f87=[%7],0x80\n;;"

			"ldf.fill f88=[%0],0x80\n"
			"ldf.fill f89=[%1],0x80\n"
			"ldf.fill f90=[%2],0x80\n"
			"ldf.fill f91=[%3],0x80\n"
			"ldf.fill f92=[%4],0x80\n"
			"ldf.fill f93=[%5],0x80\n"
			"ldf.fill f94=[%6],0x80\n"
			"ldf.fill f95=[%7],0x80\n;;"


			"ldf.fill f96=[%0],0x80\n"
			"ldf.fill f97=[%1],0x80\n"
			"ldf.fill f98=[%2],0x80\n"
			"ldf.fill f99=[%3],0x80\n"
			"ldf.fill f100=[%4],0x80\n"
			"ldf.fill f101=[%5],0x80\n"
			"ldf.fill f102=[%6],0x80\n"
			"ldf.fill f103=[%7],0x80\n;;"

			"ldf.fill f104=[%0],0x80\n"
			"ldf.fill f105=[%1],0x80\n"
			"ldf.fill f106=[%2],0x80\n"
			"ldf.fill f107=[%3],0x80\n"
			"ldf.fill f108=[%4],0x80\n"
			"ldf.fill f109=[%5],0x80\n"
			"ldf.fill f110=[%6],0x80\n"
			"ldf.fill f111=[%7],0x80\n;;"

			"ldf.fill f112=[%0],0x80\n"
			"ldf.fill f113=[%1],0x80\n"
			"ldf.fill f114=[%2],0x80\n"
			"ldf.fill f115=[%3],0x80\n"
			"ldf.fill f116=[%4],0x80\n"
			"ldf.fill f117=[%5],0x80\n"
			"ldf.fill f118=[%6],0x80\n"
			"ldf.fill f119=[%7],0x80\n;;"

			"ldf.fill f120=[%0],0x80\n"
			"ldf.fill f121=[%1],0x80\n"
			"ldf.fill f122=[%2],0x80\n"
			"ldf.fill f123=[%3],0x80\n"
			"ldf.fill f124=[%4],0x80\n"
			"ldf.fill f125=[%5],0x80\n"
			"ldf.fill f126=[%6],0x80\n"
			"ldf.fill f127=[%7],0x80\n;;"


			:
			:"r" (&((fctx->fr)[0])),"r" (&((fctx->fr)[1])),"r" (&((fctx->fr)[2])),"r" (&((fctx->fr)[3])),
			 "r" (&((fctx->fr)[4])),"r" (&((fctx->fr)[5])),"r" (&((fctx->fr)[6])),"r" (&((fctx->fr)[7]))
		); 


}



