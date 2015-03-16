/*
 * Copyright (c) 2014 Martin Decky
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
#include <math.h>
#include "../tester.h"

#define OPERANDS   10
#define PRECISION  100000000

static double arguments[OPERANDS] = {
	3.5, -2.1, 100.0, 50.0, -1024.0, 0.0, 768.3156, 1080.499999, -600.0, 1.0
};

static double results_trunc[OPERANDS] = {
	3.0, -2.0, 100.0, 50.0, -1024.0, 0.0, 768.0, 1080.0, -600.0, 1.0
};

static double results_sin[OPERANDS] = {
	-0.350783227690, -0.863209366649, -0.506365641110, -0.262374853704,
	0.158533380044, 0.0, 0.980815184715, -0.206379975025, -0.044182448332,
	0.841470984808
};

static double results_cos[OPERANDS] = {
	-0.936456687291, -0.504846104600, 0.862318872288, 0.964966028492,
	0.987353618220, 1.0, -0.194939922623, 0.978471923925, -0.999023478833,
	0.540302305868
};

const char *test_float2(void)
{
	bool fail = false;
	
	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = trunc(arguments[i]);
		int64_t res_int = (int64_t) (res * PRECISION);
		int64_t corr_int = (int64_t) (results_trunc[i] * PRECISION);
		
		if (res_int != corr_int) {
			TPRINTF("Double truncation failed (%" PRId64 " != %" PRId64
			    ", arg %u)\n", res_int, corr_int, i);
			fail = true;
		}
	}
	
	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = sin(arguments[i]);
		int64_t res_int = (int64_t) (res * PRECISION);
		int64_t corr_int = (int64_t) (results_sin[i] * PRECISION);
		
		if (res_int != corr_int) {
			TPRINTF("Double sine failed (%" PRId64 " != %" PRId64
			    ", arg %u)\n", res_int, corr_int, i);
			fail = true;
		}
	}
	
	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = cos(arguments[i]);
		int64_t res_int = (int64_t) (res * PRECISION);
		int64_t corr_int = (int64_t) (results_cos[i] * PRECISION);
		
		if (res_int != corr_int) {
			TPRINTF("Double cosine failed (%" PRId64 " != %" PRId64
			    ", arg %u)\n", res_int, corr_int, i);
			fail = true;
		}
	}
	
	if (fail)
		return "Floating point imprecision";
	
	return NULL;
}
