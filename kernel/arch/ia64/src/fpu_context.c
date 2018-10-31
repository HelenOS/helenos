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

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 *
 */

#include <fpu_context.h>
#include <arch/register.h>

void fpu_context_save(fpu_context_t *fctx)
{
	asm volatile (
	    "stf.spill %[f32] = f32\n"
	    "stf.spill %[f33] = f33\n"
	    "stf.spill %[f34] = f34\n"
	    "stf.spill %[f35] = f35\n"
	    "stf.spill %[f36] = f36\n"
	    "stf.spill %[f37] = f37\n"
	    "stf.spill %[f38] = f38\n"
	    "stf.spill %[f39] = f39\n"
	    "stf.spill %[f40] = f40\n"
	    "stf.spill %[f41] = f41\n"
	    "stf.spill %[f42] = f42\n"
	    "stf.spill %[f43] = f43\n"
	    "stf.spill %[f44] = f44\n"
	    "stf.spill %[f45] = f45\n"
	    "stf.spill %[f46] = f46\n"
	    "stf.spill %[f47] = f47\n"
	    "stf.spill %[f48] = f48\n"
	    "stf.spill %[f49] = f49\n"
	    "stf.spill %[f50] = f50\n"
	    "stf.spill %[f51] = f51\n"
	    "stf.spill %[f52] = f52\n"
	    "stf.spill %[f53] = f53\n"
	    "stf.spill %[f54] = f54\n"
	    "stf.spill %[f55] = f55\n"
	    "stf.spill %[f56] = f56\n"
	    "stf.spill %[f57] = f57\n"
	    "stf.spill %[f58] = f58\n"
	    "stf.spill %[f59] = f59\n"
	    "stf.spill %[f60] = f60\n"
	    "stf.spill %[f61] = f61\n"
	    :
	      [f32] "=m" (fctx->fr[0]),
	      [f33] "=m" (fctx->fr[1]),
	      [f34] "=m" (fctx->fr[2]),
	      [f35] "=m" (fctx->fr[3]),
	      [f36] "=m" (fctx->fr[4]),
	      [f37] "=m" (fctx->fr[5]),
	      [f38] "=m" (fctx->fr[6]),
	      [f39] "=m" (fctx->fr[7]),
	      [f40] "=m" (fctx->fr[8]),
	      [f41] "=m" (fctx->fr[9]),
	      [f42] "=m" (fctx->fr[10]),
	      [f43] "=m" (fctx->fr[11]),
	      [f44] "=m" (fctx->fr[12]),
	      [f45] "=m" (fctx->fr[13]),
	      [f46] "=m" (fctx->fr[14]),
	      [f47] "=m" (fctx->fr[15]),
	      [f48] "=m" (fctx->fr[16]),
	      [f49] "=m" (fctx->fr[17]),
	      [f50] "=m" (fctx->fr[18]),
	      [f51] "=m" (fctx->fr[19]),
	      [f52] "=m" (fctx->fr[20]),
	      [f53] "=m" (fctx->fr[21]),
	      [f54] "=m" (fctx->fr[22]),
	      [f55] "=m" (fctx->fr[23]),
	      [f56] "=m" (fctx->fr[24]),
	      [f57] "=m" (fctx->fr[25]),
	      [f58] "=m" (fctx->fr[26]),
	      [f59] "=m" (fctx->fr[27]),
	      [f60] "=m" (fctx->fr[28]),
	      [f61] "=m" (fctx->fr[29])
	);

	asm volatile (
	    "stf.spill %[f62] = f62\n"
	    "stf.spill %[f63] = f63\n"
	    "stf.spill %[f64] = f64\n"
	    "stf.spill %[f65] = f65\n"
	    "stf.spill %[f66] = f66\n"
	    "stf.spill %[f67] = f67\n"
	    "stf.spill %[f68] = f68\n"
	    "stf.spill %[f69] = f69\n"
	    "stf.spill %[f70] = f70\n"
	    "stf.spill %[f71] = f71\n"
	    "stf.spill %[f72] = f72\n"
	    "stf.spill %[f73] = f73\n"
	    "stf.spill %[f74] = f74\n"
	    "stf.spill %[f75] = f75\n"
	    "stf.spill %[f76] = f76\n"
	    "stf.spill %[f77] = f77\n"
	    "stf.spill %[f78] = f78\n"
	    "stf.spill %[f79] = f79\n"
	    "stf.spill %[f80] = f80\n"
	    "stf.spill %[f81] = f81\n"
	    "stf.spill %[f82] = f82\n"
	    "stf.spill %[f83] = f83\n"
	    "stf.spill %[f84] = f84\n"
	    "stf.spill %[f85] = f85\n"
	    "stf.spill %[f86] = f86\n"
	    "stf.spill %[f87] = f87\n"
	    "stf.spill %[f88] = f88\n"
	    "stf.spill %[f89] = f89\n"
	    "stf.spill %[f90] = f90\n"
	    "stf.spill %[f91] = f91\n"
	    :
	      [f62] "=m" (fctx->fr[30]),
	      [f63] "=m" (fctx->fr[31]),
	      [f64] "=m" (fctx->fr[32]),
	      [f65] "=m" (fctx->fr[33]),
	      [f66] "=m" (fctx->fr[34]),
	      [f67] "=m" (fctx->fr[35]),
	      [f68] "=m" (fctx->fr[36]),
	      [f69] "=m" (fctx->fr[37]),
	      [f70] "=m" (fctx->fr[38]),
	      [f71] "=m" (fctx->fr[39]),
	      [f72] "=m" (fctx->fr[40]),
	      [f73] "=m" (fctx->fr[41]),
	      [f74] "=m" (fctx->fr[42]),
	      [f75] "=m" (fctx->fr[43]),
	      [f76] "=m" (fctx->fr[44]),
	      [f77] "=m" (fctx->fr[45]),
	      [f78] "=m" (fctx->fr[46]),
	      [f79] "=m" (fctx->fr[47]),
	      [f80] "=m" (fctx->fr[48]),
	      [f81] "=m" (fctx->fr[49]),
	      [f82] "=m" (fctx->fr[50]),
	      [f83] "=m" (fctx->fr[51]),
	      [f84] "=m" (fctx->fr[52]),
	      [f85] "=m" (fctx->fr[53]),
	      [f86] "=m" (fctx->fr[54]),
	      [f87] "=m" (fctx->fr[55]),
	      [f88] "=m" (fctx->fr[56]),
	      [f89] "=m" (fctx->fr[57]),
	      [f90] "=m" (fctx->fr[58]),
	      [f91] "=m" (fctx->fr[59])
	);

	asm volatile (
	    "stf.spill %[f92] = f92\n"
	    "stf.spill %[f93] = f93\n"
	    "stf.spill %[f94] = f94\n"
	    "stf.spill %[f95] = f95\n"
	    "stf.spill %[f96] = f96\n"
	    "stf.spill %[f97] = f97\n"
	    "stf.spill %[f98] = f98\n"
	    "stf.spill %[f99] = f99\n"
	    "stf.spill %[f100] = f100\n"
	    "stf.spill %[f101] = f101\n"
	    "stf.spill %[f102] = f102\n"
	    "stf.spill %[f103] = f103\n"
	    "stf.spill %[f104] = f104\n"
	    "stf.spill %[f105] = f105\n"
	    "stf.spill %[f106] = f106\n"
	    "stf.spill %[f107] = f107\n"
	    "stf.spill %[f108] = f108\n"
	    "stf.spill %[f109] = f109\n"
	    "stf.spill %[f110] = f110\n"
	    "stf.spill %[f111] = f111\n"
	    "stf.spill %[f112] = f112\n"
	    "stf.spill %[f113] = f113\n"
	    "stf.spill %[f114] = f114\n"
	    "stf.spill %[f115] = f115\n"
	    "stf.spill %[f116] = f116\n"
	    "stf.spill %[f117] = f117\n"
	    "stf.spill %[f118] = f118\n"
	    "stf.spill %[f119] = f119\n"
	    "stf.spill %[f120] = f120\n"
	    "stf.spill %[f121] = f121\n"
	    :
	      [f92] "=m" (fctx->fr[60]),
	      [f93] "=m" (fctx->fr[61]),
	      [f94] "=m" (fctx->fr[62]),
	      [f95] "=m" (fctx->fr[63]),
	      [f96] "=m" (fctx->fr[64]),
	      [f97] "=m" (fctx->fr[65]),
	      [f98] "=m" (fctx->fr[66]),
	      [f99] "=m" (fctx->fr[67]),
	      [f100] "=m" (fctx->fr[68]),
	      [f101] "=m" (fctx->fr[69]),
	      [f102] "=m" (fctx->fr[70]),
	      [f103] "=m" (fctx->fr[71]),
	      [f104] "=m" (fctx->fr[72]),
	      [f105] "=m" (fctx->fr[73]),
	      [f106] "=m" (fctx->fr[74]),
	      [f107] "=m" (fctx->fr[75]),
	      [f108] "=m" (fctx->fr[76]),
	      [f109] "=m" (fctx->fr[77]),
	      [f110] "=m" (fctx->fr[78]),
	      [f111] "=m" (fctx->fr[79]),
	      [f112] "=m" (fctx->fr[80]),
	      [f113] "=m" (fctx->fr[81]),
	      [f114] "=m" (fctx->fr[82]),
	      [f115] "=m" (fctx->fr[83]),
	      [f116] "=m" (fctx->fr[84]),
	      [f117] "=m" (fctx->fr[85]),
	      [f118] "=m" (fctx->fr[86]),
	      [f119] "=m" (fctx->fr[87]),
	      [f120] "=m" (fctx->fr[88]),
	      [f121] "=m" (fctx->fr[89])
	);

	asm volatile (
	    "stf.spill %[f122] = f122\n"
	    "stf.spill %[f123] = f123\n"
	    "stf.spill %[f124] = f124\n"
	    "stf.spill %[f125] = f125\n"
	    "stf.spill %[f126] = f126\n"
	    "stf.spill %[f127] = f127\n"
	    :
	      [f122] "=m" (fctx->fr[90]),
	      [f123] "=m" (fctx->fr[91]),
	      [f124] "=m" (fctx->fr[92]),
	      [f125] "=m" (fctx->fr[93]),
	      [f126] "=m" (fctx->fr[94]),
	      [f127] "=m" (fctx->fr[95])
	);
}

void fpu_context_restore(fpu_context_t *fctx)
{
	asm volatile (
	    "ldf.fill f32 = %[f32]\n"
	    "ldf.fill f33 = %[f33]\n"
	    "ldf.fill f34 = %[f34]\n"
	    "ldf.fill f35 = %[f35]\n"
	    "ldf.fill f36 = %[f36]\n"
	    "ldf.fill f37 = %[f37]\n"
	    "ldf.fill f38 = %[f38]\n"
	    "ldf.fill f39 = %[f39]\n"
	    "ldf.fill f40 = %[f40]\n"
	    "ldf.fill f41 = %[f41]\n"
	    "ldf.fill f42 = %[f42]\n"
	    "ldf.fill f43 = %[f43]\n"
	    "ldf.fill f44 = %[f44]\n"
	    "ldf.fill f45 = %[f45]\n"
	    "ldf.fill f46 = %[f46]\n"
	    "ldf.fill f47 = %[f47]\n"
	    "ldf.fill f48 = %[f48]\n"
	    "ldf.fill f49 = %[f49]\n"
	    "ldf.fill f50 = %[f50]\n"
	    "ldf.fill f51 = %[f51]\n"
	    "ldf.fill f52 = %[f52]\n"
	    "ldf.fill f53 = %[f53]\n"
	    "ldf.fill f54 = %[f54]\n"
	    "ldf.fill f55 = %[f55]\n"
	    "ldf.fill f56 = %[f56]\n"
	    "ldf.fill f57 = %[f57]\n"
	    "ldf.fill f58 = %[f58]\n"
	    "ldf.fill f59 = %[f59]\n"
	    "ldf.fill f60 = %[f60]\n"
	    "ldf.fill f61 = %[f61]\n"
	    ::
	      [f32] "m" (fctx->fr[0]),
	      [f33] "m" (fctx->fr[1]),
	      [f34] "m" (fctx->fr[2]),
	      [f35] "m" (fctx->fr[3]),
	      [f36] "m" (fctx->fr[4]),
	      [f37] "m" (fctx->fr[5]),
	      [f38] "m" (fctx->fr[6]),
	      [f39] "m" (fctx->fr[7]),
	      [f40] "m" (fctx->fr[8]),
	      [f41] "m" (fctx->fr[9]),
	      [f42] "m" (fctx->fr[10]),
	      [f43] "m" (fctx->fr[11]),
	      [f44] "m" (fctx->fr[12]),
	      [f45] "m" (fctx->fr[13]),
	      [f46] "m" (fctx->fr[14]),
	      [f47] "m" (fctx->fr[15]),
	      [f48] "m" (fctx->fr[16]),
	      [f49] "m" (fctx->fr[17]),
	      [f50] "m" (fctx->fr[18]),
	      [f51] "m" (fctx->fr[19]),
	      [f52] "m" (fctx->fr[20]),
	      [f53] "m" (fctx->fr[21]),
	      [f54] "m" (fctx->fr[22]),
	      [f55] "m" (fctx->fr[23]),
	      [f56] "m" (fctx->fr[24]),
	      [f57] "m" (fctx->fr[25]),
	      [f58] "m" (fctx->fr[26]),
	      [f59] "m" (fctx->fr[27]),
	      [f60] "m" (fctx->fr[28]),
	      [f61] "m" (fctx->fr[29])
	);

	asm volatile (
	    "ldf.fill f62 = %[f62]\n"
	    "ldf.fill f63 = %[f63]\n"
	    "ldf.fill f64 = %[f64]\n"
	    "ldf.fill f65 = %[f65]\n"
	    "ldf.fill f66 = %[f66]\n"
	    "ldf.fill f67 = %[f67]\n"
	    "ldf.fill f68 = %[f68]\n"
	    "ldf.fill f69 = %[f69]\n"
	    "ldf.fill f70 = %[f70]\n"
	    "ldf.fill f71 = %[f71]\n"
	    "ldf.fill f72 = %[f72]\n"
	    "ldf.fill f73 = %[f73]\n"
	    "ldf.fill f74 = %[f74]\n"
	    "ldf.fill f75 = %[f75]\n"
	    "ldf.fill f76 = %[f76]\n"
	    "ldf.fill f77 = %[f77]\n"
	    "ldf.fill f78 = %[f78]\n"
	    "ldf.fill f79 = %[f79]\n"
	    "ldf.fill f80 = %[f80]\n"
	    "ldf.fill f81 = %[f81]\n"
	    "ldf.fill f82 = %[f82]\n"
	    "ldf.fill f83 = %[f83]\n"
	    "ldf.fill f84 = %[f84]\n"
	    "ldf.fill f85 = %[f85]\n"
	    "ldf.fill f86 = %[f86]\n"
	    "ldf.fill f87 = %[f87]\n"
	    "ldf.fill f88 = %[f88]\n"
	    "ldf.fill f89 = %[f89]\n"
	    "ldf.fill f90 = %[f90]\n"
	    "ldf.fill f91 = %[f91]\n"
	    ::
	      [f62] "m" (fctx->fr[30]),
	      [f63] "m" (fctx->fr[31]),
	      [f64] "m" (fctx->fr[32]),
	      [f65] "m" (fctx->fr[33]),
	      [f66] "m" (fctx->fr[34]),
	      [f67] "m" (fctx->fr[35]),
	      [f68] "m" (fctx->fr[36]),
	      [f69] "m" (fctx->fr[37]),
	      [f70] "m" (fctx->fr[38]),
	      [f71] "m" (fctx->fr[39]),
	      [f72] "m" (fctx->fr[40]),
	      [f73] "m" (fctx->fr[41]),
	      [f74] "m" (fctx->fr[42]),
	      [f75] "m" (fctx->fr[43]),
	      [f76] "m" (fctx->fr[44]),
	      [f77] "m" (fctx->fr[45]),
	      [f78] "m" (fctx->fr[46]),
	      [f79] "m" (fctx->fr[47]),
	      [f80] "m" (fctx->fr[48]),
	      [f81] "m" (fctx->fr[49]),
	      [f82] "m" (fctx->fr[50]),
	      [f83] "m" (fctx->fr[51]),
	      [f84] "m" (fctx->fr[52]),
	      [f85] "m" (fctx->fr[53]),
	      [f86] "m" (fctx->fr[54]),
	      [f87] "m" (fctx->fr[55]),
	      [f88] "m" (fctx->fr[56]),
	      [f89] "m" (fctx->fr[57]),
	      [f90] "m" (fctx->fr[58]),
	      [f91] "m" (fctx->fr[59])
	);

	asm volatile (
	    "ldf.fill f92 = %[f92]\n"
	    "ldf.fill f93 = %[f93]\n"
	    "ldf.fill f94 = %[f94]\n"
	    "ldf.fill f95 = %[f95]\n"
	    "ldf.fill f96 = %[f96]\n"
	    "ldf.fill f97 = %[f97]\n"
	    "ldf.fill f98 = %[f98]\n"
	    "ldf.fill f99 = %[f99]\n"
	    "ldf.fill f100 = %[f100]\n"
	    "ldf.fill f101 = %[f101]\n"
	    "ldf.fill f102 = %[f102]\n"
	    "ldf.fill f103 = %[f103]\n"
	    "ldf.fill f104 = %[f104]\n"
	    "ldf.fill f105 = %[f105]\n"
	    "ldf.fill f106 = %[f106]\n"
	    "ldf.fill f107 = %[f107]\n"
	    "ldf.fill f108 = %[f108]\n"
	    "ldf.fill f109 = %[f109]\n"
	    "ldf.fill f110 = %[f110]\n"
	    "ldf.fill f111 = %[f111]\n"
	    "ldf.fill f112 = %[f112]\n"
	    "ldf.fill f113 = %[f113]\n"
	    "ldf.fill f114 = %[f114]\n"
	    "ldf.fill f115 = %[f115]\n"
	    "ldf.fill f116 = %[f116]\n"
	    "ldf.fill f117 = %[f117]\n"
	    "ldf.fill f118 = %[f118]\n"
	    "ldf.fill f119 = %[f119]\n"
	    "ldf.fill f120 = %[f120]\n"
	    "ldf.fill f121 = %[f121]\n"
	    ::
	      [f92] "m" (fctx->fr[60]),
	      [f93] "m" (fctx->fr[61]),
	      [f94] "m" (fctx->fr[62]),
	      [f95] "m" (fctx->fr[63]),
	      [f96] "m" (fctx->fr[64]),
	      [f97] "m" (fctx->fr[65]),
	      [f98] "m" (fctx->fr[66]),
	      [f99] "m" (fctx->fr[67]),
	      [f100] "m" (fctx->fr[68]),
	      [f101] "m" (fctx->fr[69]),
	      [f102] "m" (fctx->fr[70]),
	      [f103] "m" (fctx->fr[71]),
	      [f104] "m" (fctx->fr[72]),
	      [f105] "m" (fctx->fr[73]),
	      [f106] "m" (fctx->fr[74]),
	      [f107] "m" (fctx->fr[75]),
	      [f108] "m" (fctx->fr[76]),
	      [f109] "m" (fctx->fr[77]),
	      [f110] "m" (fctx->fr[78]),
	      [f111] "m" (fctx->fr[79]),
	      [f112] "m" (fctx->fr[80]),
	      [f113] "m" (fctx->fr[81]),
	      [f114] "m" (fctx->fr[82]),
	      [f115] "m" (fctx->fr[83]),
	      [f116] "m" (fctx->fr[84]),
	      [f117] "m" (fctx->fr[85]),
	      [f118] "m" (fctx->fr[86]),
	      [f119] "m" (fctx->fr[87]),
	      [f120] "m" (fctx->fr[88]),
	      [f121] "m" (fctx->fr[89])
	);

	asm volatile (
	    "ldf.fill f122 = %[f122]\n"
	    "ldf.fill f123 = %[f123]\n"
	    "ldf.fill f124 = %[f124]\n"
	    "ldf.fill f125 = %[f125]\n"
	    "ldf.fill f126 = %[f126]\n"
	    "ldf.fill f127 = %[f127]\n"
	    ::
	      [f122] "m" (fctx->fr[90]),
	      [f123] "m" (fctx->fr[91]),
	      [f124] "m" (fctx->fr[92]),
	      [f125] "m" (fctx->fr[93]),
	      [f126] "m" (fctx->fr[94]),
	      [f127] "m" (fctx->fr[95])
	);
}

void fpu_enable(void)
{
	asm volatile (
	    "rsm %0 ;;"
	    "srlz.i\n"
	    "srlz.d ;;\n"
	    :
	    : "i" (PSR_DFH_MASK)
	);
}

void fpu_disable(void)
{
	asm volatile (
	    "ssm %0 ;;\n"
	    "srlz.i\n"
	    "srlz.d ;;\n"
	    :
	    : "i" (PSR_DFH_MASK)
	);
}

void fpu_init(void)
{
	uint64_t a = 0;

	asm volatile (
	    "mov %0 = ar.fpsr ;;\n"
	    "or %0 = %0,%1 ;;\n"
	    "mov ar.fpsr = %0 ;;\n"
	    : "+r" (a)
	    : "r" (FPSR_TRAPS_ALL | FPSR_SF1_CTRL)
	);

	asm volatile (
	    "mov f2 = f0\n"
	    "mov f3 = f0\n"
	    "mov f4 = f0\n"
	    "mov f5 = f0\n"
	    "mov f6 = f0\n"
	    "mov f7 = f0\n"
	    "mov f8 = f0\n"
	    "mov f9 = f0\n"

	    "mov f10 = f0\n"
	    "mov f11 = f0\n"
	    "mov f12 = f0\n"
	    "mov f13 = f0\n"
	    "mov f14 = f0\n"
	    "mov f15 = f0\n"
	    "mov f16 = f0\n"
	    "mov f17 = f0\n"
	    "mov f18 = f0\n"
	    "mov f19 = f0\n"

	    "mov f20 = f0\n"
	    "mov f21 = f0\n"
	    "mov f22 = f0\n"
	    "mov f23 = f0\n"
	    "mov f24 = f0\n"
	    "mov f25 = f0\n"
	    "mov f26 = f0\n"
	    "mov f27 = f0\n"
	    "mov f28 = f0\n"
	    "mov f29 = f0\n"

	    "mov f30 = f0\n"
	    "mov f31 = f0\n"
	    "mov f32 = f0\n"
	    "mov f33 = f0\n"
	    "mov f34 = f0\n"
	    "mov f35 = f0\n"
	    "mov f36 = f0\n"
	    "mov f37 = f0\n"
	    "mov f38 = f0\n"
	    "mov f39 = f0\n"

	    "mov f40 = f0\n"
	    "mov f41 = f0\n"
	    "mov f42 = f0\n"
	    "mov f43 = f0\n"
	    "mov f44 = f0\n"
	    "mov f45 = f0\n"
	    "mov f46 = f0\n"
	    "mov f47 = f0\n"
	    "mov f48 = f0\n"
	    "mov f49 = f0\n"

	    "mov f50 = f0\n"
	    "mov f51 = f0\n"
	    "mov f52 = f0\n"
	    "mov f53 = f0\n"
	    "mov f54 = f0\n"
	    "mov f55 = f0\n"
	    "mov f56 = f0\n"
	    "mov f57 = f0\n"
	    "mov f58 = f0\n"
	    "mov f59 = f0\n"

	    "mov f60 = f0\n"
	    "mov f61 = f0\n"
	    "mov f62 = f0\n"
	    "mov f63 = f0\n"
	    "mov f64 = f0\n"
	    "mov f65 = f0\n"
	    "mov f66 = f0\n"
	    "mov f67 = f0\n"
	    "mov f68 = f0\n"
	    "mov f69 = f0\n"

	    "mov f70 = f0\n"
	    "mov f71 = f0\n"
	    "mov f72 = f0\n"
	    "mov f73 = f0\n"
	    "mov f74 = f0\n"
	    "mov f75 = f0\n"
	    "mov f76 = f0\n"
	    "mov f77 = f0\n"
	    "mov f78 = f0\n"
	    "mov f79 = f0\n"

	    "mov f80 = f0\n"
	    "mov f81 = f0\n"
	    "mov f82 = f0\n"
	    "mov f83 = f0\n"
	    "mov f84 = f0\n"
	    "mov f85 = f0\n"
	    "mov f86 = f0\n"
	    "mov f87 = f0\n"
	    "mov f88 = f0\n"
	    "mov f89 = f0\n"

	    "mov f90 = f0\n"
	    "mov f91 = f0\n"
	    "mov f92 = f0\n"
	    "mov f93 = f0\n"
	    "mov f94 = f0\n"
	    "mov f95 = f0\n"
	    "mov f96 = f0\n"
	    "mov f97 = f0\n"
	    "mov f98 = f0\n"
	    "mov f99 = f0\n"

	    "mov f100 = f0\n"
	    "mov f101 = f0\n"
	    "mov f102 = f0\n"
	    "mov f103 = f0\n"
	    "mov f104 = f0\n"
	    "mov f105 = f0\n"
	    "mov f106 = f0\n"
	    "mov f107 = f0\n"
	    "mov f108 = f0\n"
	    "mov f109 = f0\n"

	    "mov f110 = f0\n"
	    "mov f111 = f0\n"
	    "mov f112 = f0\n"
	    "mov f113 = f0\n"
	    "mov f114 = f0\n"
	    "mov f115 = f0\n"
	    "mov f116 = f0\n"
	    "mov f117 = f0\n"
	    "mov f118 = f0\n"
	    "mov f119 = f0\n"

	    "mov f120 = f0\n"
	    "mov f121 = f0\n"
	    "mov f122 = f0\n"
	    "mov f123 = f0\n"
	    "mov f124 = f0\n"
	    "mov f125 = f0\n"
	    "mov f126 = f0\n"
	    "mov f127 = f0\n"
	);

}

/** @}
 */
