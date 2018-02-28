/*
 * Copyright (c) 2013 Vojtech Horky
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

#include <bits.h>

/** Compute number of trailing 0-bits in a number. */
int __ctzdi2(long a)
{
	unsigned int bits = 0;
	while (((a >> bits) & 1) == 0) {
		bits++;
		if (bits >= sizeof(a) * 8) {
			break;
		}
	}

	return bits;
}

/** Compute number of trailing 0-bits in a number. */
int __ctzsi2(int a)
{
	unsigned int bits = 0;
	while (((a >> bits) & 1) == 0) {
		bits++;
		if (bits >= sizeof(a) * 8) {
			break;
		}
	}

	return bits;
}

/** Compute number of leading 0-bits in a number. */
int __clzdi2(long a)
{
	int index = sizeof(a) * 8 - 1;
	int bits = 0;
	while (index >= 0) {
		if (((a >> index) & 1) == 0) {
			bits++;
		} else {
			break;
		}
		index--;
	}

	return bits;
}

/** Compute index of the first 1-bit in a number increased by one.
 *
 * If the number is zero, zero is returned.
 */
int __ffsdi2(long a) {
	if (a == 0) {
		return 0;
	}

	return 1 + __ctzdi2(a);
}

/** Compute number of set bits in a number. */
int __popcountsi2(int a)
{
	int bits = 0;
	for (unsigned int i = 0; i < sizeof(a) * 8; i++)	 {
		if (((a >> i) & 1) != 0) {
			bits++;
		}
	}
	return bits;
}

/** Compute number of set bits in a number. */
int __popcountdi2(long a)
{
	int bits = 0;
	for (unsigned int i = 0; i < sizeof(a) * 8; i++)	 {
		if (((a >> i) & 1) != 0) {
			bits++;
		}
	}
	return bits;
}

/** @}
 */
