/*
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup softint
 * @{
 */
/**
 * @file Logical and arithmetic shifts.
 */

#include <shift.h>
#include <lltype.h>

long long __ashldi3 (long long val, int shift)
{
	union lltype ll;

	ll.s_whole = val;

	if (shift <= 0) {
		return ll.s_whole;
	}

	if (shift >= (int) WHOLE_BIT_CNT) {
		ll.u_half[HI] = 0;
		ll.u_half[LO] = 0;
		return ll.s_whole;
	}

	if (shift >= (int) HALF_BIT_CNT) {
		ll.u_half[HI] = ll.u_half[LO] << (shift - HALF_BIT_CNT);
		ll.u_half[LO] = 0;
	} else {
		ll.u_half[HI] <<= shift;
		ll.u_half[HI] |= ll.u_half[LO] >> (HALF_BIT_CNT - shift);
		ll.u_half[LO] <<= shift;
	}

	return ll.s_whole;
}

long long __ashrdi3 (long long val, int shift)
{
	union lltype ll;

	ll.s_whole = val;

	if (shift <= 0) {
		return ll.s_whole;
	}

	long fill = ll.s_half[HI] >> (HALF_BIT_CNT - 1);

	if (shift >= (int) WHOLE_BIT_CNT) {
		ll.s_half[HI] = fill;
		ll.s_half[LO] = fill;
		return ll.s_whole;
	}

	if (shift >= (int) HALF_BIT_CNT) {
		ll.s_half[LO] = ll.s_half[HI] >> (shift - HALF_BIT_CNT);
		ll.s_half[HI] = fill;
	} else {
		ll.u_half[LO] >>= shift;
		ll.u_half[LO] |= ll.u_half[HI] << (HALF_BIT_CNT - shift);
		ll.s_half[HI] >>= shift;
	}

	return ll.s_whole;
}

long long __lshrdi3 (long long val, int shift)
{
	union lltype ll;

	ll.s_whole = val;

	if (shift <= 0) {
		return ll.s_whole;
	}

	if (shift >= (int) WHOLE_BIT_CNT) {
		ll.u_half[HI] = 0;
		ll.u_half[LO] = 0;
		return ll.s_whole;
	}

	if (shift >= (int) HALF_BIT_CNT) {
		ll.u_half[LO] = ll.u_half[HI] >> (shift - HALF_BIT_CNT);
		ll.u_half[HI] = 0;
	} else {
		ll.u_half[LO] >>= shift;
		ll.u_half[LO] |= ll.u_half[HI] << (HALF_BIT_CNT - shift);
		ll.u_half[HI] >>= shift;
	}

	return ll.s_whole;
}

long long __aeabi_llsl(long long val, int shift)
{
	return __ashldi3(val, shift);
}

long long __aeabi_llsr(long long val, int shift)
{
	return __lshrdi3(val, shift);
}

/** @}
 */
