/*
 * Copyright (c) 2012 Martin Decky
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

#include <stdio.h>
#include <stdlib.h>
#include <sftypes.h>
#include <add.h>
#include <sub.h>
#include <bool.h>
#include "../tester.h"

#define OPERANDS   5
#define PRECISION  10000

#define PRIdCMPTYPE  PRId32

typedef int32_t cmptype_t;

static float float_op_a[OPERANDS] =
	{3.5, -2.1, 100.0, 50.0, -1024.0};

static float float_op_b[OPERANDS] =
	{-2.1, 100.0, 50.0, -1024.0, 3.5};

static cmptype_t cmpabs(cmptype_t a)
{
	if (a >= 0)
		return a;
	
	return -a;
}

static bool test_float_add(void)
{
	bool correct = true;
	
	for (unsigned int i = 0; i < OPERANDS; i++) {
		for (unsigned int j = 0; j < OPERANDS; j++) {
			float a = float_op_a[i];
			float b = float_op_b[j];
			float c = a + b;
			
			float_t sa;
			float_t sb;
			float_t sc;
			
			sa.val = float_op_a[i];
			sb.val = float_op_b[j];
			if (sa.data.parts.sign == sb.data.parts.sign)
				sc.data = add_float(sa.data, sb.data);
			else if (sa.data.parts.sign) {
				sa.data.parts.sign = 0;
				sc.data = sub_float(sb.data, sa.data);
			} else {
				sb.data.parts.sign = 0;
				sc.data = sub_float(sa.data, sb.data);
			}
				
			cmptype_t ic = (cmptype_t) (c * PRECISION);
			cmptype_t isc = (cmptype_t) (sc.val * PRECISION);
			cmptype_t diff = cmpabs(ic - isc);
			
			if (diff != 0) {
				TPRINTF("i=%u, j=%u diff=%" PRIdCMPTYPE "\n", i, j, diff);
				correct = false;
			}
		}
	}
	
	return correct;
}

const char *test_softfloat1(void)
{
	if (!test_float_add())
		return "Float addition failed";
	
	return NULL;
}
