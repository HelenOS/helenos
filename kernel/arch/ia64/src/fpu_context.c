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
 *
 */

#include <fpu_context.h>
#include <arch/register.h>
#include <print.h>

void fpu_context_save(fpu_context_t *fctx)
{
	asm volatile (
		"stf.spill [%0] = f32, 0x80\n"
		"stf.spill [%1] = f33, 0x80\n"
		"stf.spill [%2] = f34, 0x80\n"
		"stf.spill [%3] = f35, 0x80\n"
		"stf.spill [%4] = f36, 0x80\n"
		"stf.spill [%5] = f37, 0x80\n"
		"stf.spill [%6] = f38, 0x80\n"
		"stf.spill [%7] = f39, 0x80\n;;"

		"stf.spill [%0] = f40, 0x80\n"
		"stf.spill [%1] = f41, 0x80\n"
		"stf.spill [%2] = f42, 0x80\n"
		"stf.spill [%3] = f43, 0x80\n"
		"stf.spill [%4] = f44, 0x80\n"
		"stf.spill [%5] = f45, 0x80\n"
		"stf.spill [%6] = f46, 0x80\n"
		"stf.spill [%7] = f47, 0x80\n;;"

		"stf.spill [%0] = f48, 0x80\n"
		"stf.spill [%1] = f49, 0x80\n"
		"stf.spill [%2] = f50, 0x80\n"
		"stf.spill [%3] = f51, 0x80\n"
		"stf.spill [%4] = f52, 0x80\n"
		"stf.spill [%5] = f53, 0x80\n"
		"stf.spill [%6] = f54, 0x80\n"
		"stf.spill [%7] = f55, 0x80\n;;"

		"stf.spill [%0] = f56, 0x80\n"
		"stf.spill [%1] = f57, 0x80\n"
		"stf.spill [%2] = f58, 0x80\n"
		"stf.spill [%3] = f59, 0x80\n"
		"stf.spill [%4] = f60, 0x80\n"
		"stf.spill [%5] = f61, 0x80\n"
		"stf.spill [%6] = f62, 0x80\n"
		"stf.spill [%7] = f63, 0x80\n;;"

		"stf.spill [%0] = f64, 0x80\n"
		"stf.spill [%1] = f65, 0x80\n"
		"stf.spill [%2] = f66, 0x80\n"
		"stf.spill [%3] = f67, 0x80\n"
		"stf.spill [%4] = f68, 0x80\n"
		"stf.spill [%5] = f69, 0x80\n"
		"stf.spill [%6] = f70, 0x80\n"
		"stf.spill [%7] = f71, 0x80\n;;"

		"stf.spill [%0] = f72, 0x80\n"
		"stf.spill [%1] = f73, 0x80\n"
		"stf.spill [%2] = f74, 0x80\n"
		"stf.spill [%3] = f75, 0x80\n"
		"stf.spill [%4] = f76, 0x80\n"
		"stf.spill [%5] = f77, 0x80\n"
		"stf.spill [%6] = f78, 0x80\n"
		"stf.spill [%7] = f79, 0x80\n;;"

		"stf.spill [%0] = f80, 0x80\n"
		"stf.spill [%1] = f81, 0x80\n"
		"stf.spill [%2] = f82, 0x80\n"
		"stf.spill [%3] = f83, 0x80\n"
		"stf.spill [%4] = f84, 0x80\n"
		"stf.spill [%5] = f85, 0x80\n"
		"stf.spill [%6] = f86, 0x80\n"
		"stf.spill [%7] = f87, 0x80\n;;"

		"stf.spill [%0] = f88, 0x80\n"
		"stf.spill [%1] = f89, 0x80\n"
		"stf.spill [%2] = f90, 0x80\n"
		"stf.spill [%3] = f91, 0x80\n"
		"stf.spill [%4] = f92, 0x80\n"
		"stf.spill [%5] = f93, 0x80\n"
		"stf.spill [%6] = f94, 0x80\n"
		"stf.spill [%7] = f95, 0x80\n;;"

		"stf.spill [%0] = f96, 0x80\n"
		"stf.spill [%1] = f97, 0x80\n"
		"stf.spill [%2] = f98, 0x80\n"
		"stf.spill [%3] = f99, 0x80\n"
		"stf.spill [%4] = f100, 0x80\n"
		"stf.spill [%5] = f101, 0x80\n"
		"stf.spill [%6] = f102, 0x80\n"
		"stf.spill [%7] = f103, 0x80\n;;"

		"stf.spill [%0] = f104, 0x80\n"
		"stf.spill [%1] = f105, 0x80\n"
		"stf.spill [%2] = f106, 0x80\n"
		"stf.spill [%3] = f107, 0x80\n"
		"stf.spill [%4] = f108, 0x80\n"
		"stf.spill [%5] = f109, 0x80\n"
		"stf.spill [%6] = f110, 0x80\n"
		"stf.spill [%7] = f111, 0x80\n;;"

		"stf.spill [%0] = f112, 0x80\n"
		"stf.spill [%1] = f113, 0x80\n"
		"stf.spill [%2] = f114, 0x80\n"
		"stf.spill [%3] = f115, 0x80\n"
		"stf.spill [%4] = f116, 0x80\n"
		"stf.spill [%5] = f117, 0x80\n"
		"stf.spill [%6] = f118, 0x80\n"
		"stf.spill [%7] = f119, 0x80\n;;"

		"stf.spill [%0] = f120, 0x80\n"
		"stf.spill [%1] = f121, 0x80\n"
		"stf.spill [%2] = f122, 0x80\n"
		"stf.spill [%3] = f123, 0x80\n"
		"stf.spill [%4] = f124, 0x80\n"
		"stf.spill [%5] = f125, 0x80\n"
		"stf.spill [%6] = f126, 0x80\n"
		"stf.spill [%7] = f127, 0x80\n;;"

		:
		: "r" (&((fctx->fr)[0])), "r" (&((fctx->fr)[1])),
		  "r" (&((fctx->fr)[2])), "r" (&((fctx->fr)[3])),
		  "r" (&((fctx->fr)[4])), "r" (&((fctx->fr)[5])),
		  "r" (&((fctx->fr)[6])), "r" (&((fctx->fr)[7]))
	); 
	
}

void fpu_context_restore(fpu_context_t *fctx)
{
	asm volatile (
		"ldf.fill f32 = [%0], 0x80\n"
		"ldf.fill f33 = [%1], 0x80\n"
		"ldf.fill f34 = [%2], 0x80\n"
		"ldf.fill f35 = [%3], 0x80\n"
		"ldf.fill f36 = [%4], 0x80\n"
		"ldf.fill f37 = [%5], 0x80\n"
		"ldf.fill f38 = [%6], 0x80\n"
		"ldf.fill f39 = [%7], 0x80\n;;"

		"ldf.fill f40 = [%0], 0x80\n"
		"ldf.fill f41 = [%1], 0x80\n"
		"ldf.fill f42 = [%2], 0x80\n"
		"ldf.fill f43 = [%3], 0x80\n"
		"ldf.fill f44 = [%4], 0x80\n"
		"ldf.fill f45 = [%5], 0x80\n"
		"ldf.fill f46 = [%6], 0x80\n"
		"ldf.fill f47 = [%7], 0x80\n;;"

		"ldf.fill f48 = [%0], 0x80\n"
		"ldf.fill f49 = [%1], 0x80\n"
		"ldf.fill f50 = [%2], 0x80\n"
		"ldf.fill f51 = [%3], 0x80\n"
		"ldf.fill f52 = [%4], 0x80\n"
		"ldf.fill f53 = [%5], 0x80\n"
		"ldf.fill f54 = [%6], 0x80\n"
		"ldf.fill f55 = [%7], 0x80\n;;"

		"ldf.fill f56 = [%0], 0x80\n"
		"ldf.fill f57 = [%1], 0x80\n"
		"ldf.fill f58 = [%2], 0x80\n"
		"ldf.fill f59 = [%3], 0x80\n"
		"ldf.fill f60 = [%4], 0x80\n"
		"ldf.fill f61 = [%5], 0x80\n"
		"ldf.fill f62 = [%6], 0x80\n"
		"ldf.fill f63 = [%7], 0x80\n;;"

		"ldf.fill f64 = [%0], 0x80\n"
		"ldf.fill f65 = [%1], 0x80\n"
		"ldf.fill f66 = [%2], 0x80\n"
		"ldf.fill f67 = [%3], 0x80\n"
		"ldf.fill f68 = [%4], 0x80\n"
		"ldf.fill f69 = [%5], 0x80\n"
		"ldf.fill f70 = [%6], 0x80\n"
		"ldf.fill f71 = [%7], 0x80\n;;"

		"ldf.fill f72 = [%0], 0x80\n"
		"ldf.fill f73 = [%1], 0x80\n"
		"ldf.fill f74 = [%2], 0x80\n"
		"ldf.fill f75 = [%3], 0x80\n"
		"ldf.fill f76 = [%4], 0x80\n"
		"ldf.fill f77 = [%5], 0x80\n"
		"ldf.fill f78 = [%6], 0x80\n"
		"ldf.fill f79 = [%7], 0x80\n;;"

		"ldf.fill f80 = [%0], 0x80\n"
		"ldf.fill f81 = [%1], 0x80\n"
		"ldf.fill f82 = [%2], 0x80\n"
		"ldf.fill f83 = [%3], 0x80\n"
		"ldf.fill f84 = [%4], 0x80\n"
		"ldf.fill f85 = [%5], 0x80\n"
		"ldf.fill f86 = [%6], 0x80\n"
		"ldf.fill f87 = [%7], 0x80\n;;"

		"ldf.fill f88 = [%0], 0x80\n"
		"ldf.fill f89 = [%1], 0x80\n"
		"ldf.fill f90 = [%2], 0x80\n"
		"ldf.fill f91 = [%3], 0x80\n"
		"ldf.fill f92 = [%4], 0x80\n"
		"ldf.fill f93 = [%5], 0x80\n"
		"ldf.fill f94 = [%6], 0x80\n"
		"ldf.fill f95 = [%7], 0x80\n;;"

		"ldf.fill f96 = [%0], 0x80\n"
		"ldf.fill f97 = [%1], 0x80\n"
		"ldf.fill f98 = [%2], 0x80\n"
		"ldf.fill f99 = [%3], 0x80\n"
		"ldf.fill f100 = [%4], 0x80\n"
		"ldf.fill f101 = [%5], 0x80\n"
		"ldf.fill f102 = [%6], 0x80\n"
		"ldf.fill f103 = [%7], 0x80\n;;"

		"ldf.fill f104 = [%0], 0x80\n"
		"ldf.fill f105 = [%1], 0x80\n"
		"ldf.fill f106 = [%2], 0x80\n"
		"ldf.fill f107 = [%3], 0x80\n"
		"ldf.fill f108 = [%4], 0x80\n"
		"ldf.fill f109 = [%5], 0x80\n"
		"ldf.fill f110 = [%6], 0x80\n"
		"ldf.fill f111 = [%7], 0x80\n;;"

		"ldf.fill f112 = [%0], 0x80\n"
		"ldf.fill f113 = [%1], 0x80\n"
		"ldf.fill f114 = [%2], 0x80\n"
		"ldf.fill f115 = [%3], 0x80\n"
		"ldf.fill f116 = [%4], 0x80\n"
		"ldf.fill f117 = [%5], 0x80\n"
		"ldf.fill f118 = [%6], 0x80\n"
		"ldf.fill f119 = [%7], 0x80\n;;"

		"ldf.fill f120 = [%0], 0x80\n"
		"ldf.fill f121 = [%1], 0x80\n"
		"ldf.fill f122 = [%2], 0x80\n"
		"ldf.fill f123 = [%3], 0x80\n"
		"ldf.fill f124 = [%4], 0x80\n"
		"ldf.fill f125 = [%5], 0x80\n"
		"ldf.fill f126 = [%6], 0x80\n"
		"ldf.fill f127 = [%7], 0x80\n;;"

		:
		: "r" (&((fctx->fr)[0])), "r" (&((fctx->fr)[1])),
		  "r" (&((fctx->fr)[2])), "r" (&((fctx->fr)[3])),
		  "r" (&((fctx->fr)[4])), "r" (&((fctx->fr)[5])),
		  "r" (&((fctx->fr)[6])), "r" (&((fctx->fr)[7]))
	); 
}

void fpu_enable(void)
{
	uint64_t a = 0;

	asm volatile (
		"rsm %0 ;;"
		"srlz.i\n"
		"srlz.d ;;\n"
		:
		: "i" (PSR_DFH_MASK)
	);

	asm volatile (
		"mov %0 = ar.fpsr ;;\n"
		"or %0 = %0,%1 ;;\n"
		"mov ar.fpsr = %0 ;;\n"
		: "+r" (a)
		: "r" (0x38)
	);
}

void fpu_disable(void)
{
	uint64_t a = 0 ;

	asm volatile (
		"ssm %0 ;;\n"
		"srlz.i\n"
		"srlz.d ;;\n"
		:
		: "i" (PSR_DFH_MASK)
	);

	asm volatile (
		"mov %0 = ar.fpsr ;;\n"
		"or %0 = %0,%1 ;;\n"
		"mov ar.fpsr = %0 ;;\n"
		: "+r" (a)
		: "r" (0x38)
	);
}

void fpu_init(void)
{
	uint64_t a = 0 ;

	asm volatile (
		"mov %0 = ar.fpsr ;;\n"
		"or %0 = %0,%1 ;;\n"
		"mov ar.fpsr = %0 ;;\n"
		: "+r" (a)
		: "r" (0x38)
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
