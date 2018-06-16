/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <stdlib.h>

static int glbl_seed = 1;

int rand(void)
{
	return glbl_seed = ((1366 * glbl_seed + 150889) % RAND_MAX);
}

void srand(unsigned int seed)
{
	glbl_seed = seed % RAND_MAX;
}

/** Compute quotient and remainder of int division.
 *
 * @param numer Numerator
 * @param denom Denominator
 * @return Structure containing quotient and remainder
 */
div_t div(int numer, int denom)
{
	div_t d;

	d.quot = numer / denom;
	d.rem = numer % denom;

	return d;
}

/** Compute quotient and remainder of long division.
 *
 * @param numer Numerator
 * @param denom Denominator
 * @return Structure containing quotient and remainder
 */
ldiv_t ldiv(long numer, long denom)
{
	ldiv_t d;

	d.quot = numer / denom;
	d.rem = numer % denom;

	return d;
}

/** Compute quotient and remainder of long long division.
 *
 * @param numer Numerator
 * @param denom Denominator
 * @return Structure containing quotient and remainder
 */
lldiv_t lldiv(long long numer, long long denom)
{
	lldiv_t d;

	d.quot = numer / denom;
	d.rem = numer % denom;

	return d;
}

/** @}
 */
