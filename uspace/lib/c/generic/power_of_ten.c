/*
 * Copyright (c) 2012 Adam Hraska
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

#include "private/power_of_ten.h"

#include <ieee_double.h>
#include <assert.h>

/** Precomputed normalized rounded-up powers of 10^k.
 *
 * The powers were computed using arbitrary precision arithmetic
 * and rounded up to the top 64 significant bits. Therefore:
 *   10^dec_exp == significand * 2^bin_exp  +/- 0.5 ulp error
 *
 * The smallest interval of binary exponents computed by hand
 * is [-1083, 987]. Add 200 (exponent change > 3 * 64 bits)
 * to both bounds just to be on the safe side; ie [-1283, 1187].
 */
static struct {
	uint64_t significand;
	int16_t bin_exp;
	int16_t dec_exp;
} fp_powers_of_10[] = {
	{ 0xb8e1cbc28bef0b69ULL, -1286, -368 },
	{ 0x89bf722840327f82ULL, -1259, -360 },
	{ 0xcd42a11346f34f7dULL, -1233, -352 },
	{ 0x98ee4a22ecf3188cULL, -1206, -344 },
	{ 0xe3e27a444d8d98b8ULL, -1180, -336 },
	{ 0xa9c98d8ccb009506ULL, -1153, -328 },
	{ 0xfd00b897478238d1ULL, -1127, -320 },
	{ 0xbc807527ed3e12bdULL, -1100, -312 },
	{ 0x8c71dcd9ba0b4926ULL, -1073, -304 },
	{ 0xd1476e2c07286faaULL, -1047, -296 },
	{ 0x9becce62836ac577ULL, -1020, -288 },
	{ 0xe858ad248f5c22caULL, -994, -280 },
	{ 0xad1c8eab5ee43b67ULL, -967, -272 },
	{ 0x80fa687f881c7f8eULL, -940, -264 },
	{ 0xc0314325637a193aULL, -914, -256 },
	{ 0x8f31cc0937ae58d3ULL, -887, -248 },
	{ 0xd5605fcdcf32e1d7ULL, -861, -240 },
	{ 0x9efa548d26e5a6e2ULL, -834, -232 },
	{ 0xece53cec4a314ebeULL, -808, -224 },
	{ 0xb080392cc4349dedULL, -781, -216 },
	{ 0x8380dea93da4bc60ULL, -754, -208 },
	{ 0xc3f490aa77bd60fdULL, -728, -200 },
	{ 0x91ff83775423cc06ULL, -701, -192 },
	{ 0xd98ddaee19068c76ULL, -675, -184 },
	{ 0xa21727db38cb0030ULL, -648, -176 },
	{ 0xf18899b1bc3f8ca2ULL, -622, -168 },
	{ 0xb3f4e093db73a093ULL, -595, -160 },
	{ 0x8613fd0145877586ULL, -568, -152 },
	{ 0xc7caba6e7c5382c9ULL, -542, -144 },
	{ 0x94db483840b717f0ULL, -515, -136 },
	{ 0xddd0467c64bce4a1ULL, -489, -128 },
	{ 0xa54394fe1eedb8ffULL, -462, -120 },
	{ 0xf64335bcf065d37dULL, -436, -112 },
	{ 0xb77ada0617e3bbcbULL, -409, -104 },
	{ 0x88b402f7fd75539bULL, -382, -96 },
	{ 0xcbb41ef979346bcaULL, -356, -88 },
	{ 0x97c560ba6b0919a6ULL, -329, -80 },
	{ 0xe2280b6c20dd5232ULL, -303, -72 },
	{ 0xa87fea27a539e9a5ULL, -276, -64 },
	{ 0xfb158592be068d2fULL, -250, -56 },
	{ 0xbb127c53b17ec159ULL, -223, -48 },
	{ 0x8b61313bbabce2c6ULL, -196, -40 },
	{ 0xcfb11ead453994baULL, -170, -32 },
	{ 0x9abe14cd44753b53ULL, -143, -24 },
	{ 0xe69594bec44de15bULL, -117, -16 },
	{ 0xabcc77118461cefdULL, -90, -8 },
	{ 0x8000000000000000ULL, -63, 0 },
	{ 0xbebc200000000000ULL, -37, 8 },
	{ 0x8e1bc9bf04000000ULL, -10, 16 },
	{ 0xd3c21bcecceda100ULL, 16, 24 },
	{ 0x9dc5ada82b70b59eULL, 43, 32 },
	{ 0xeb194f8e1ae525fdULL, 69, 40 },
	{ 0xaf298d050e4395d7ULL, 96, 48 },
	{ 0x82818f1281ed44a0ULL, 123, 56 },
	{ 0xc2781f49ffcfa6d5ULL, 149, 64 },
	{ 0x90e40fbeea1d3a4bULL, 176, 72 },
	{ 0xd7e77a8f87daf7fcULL, 202, 80 },
	{ 0xa0dc75f1778e39d6ULL, 229, 88 },
	{ 0xefb3ab16c59b14a3ULL, 255, 96 },
	{ 0xb2977ee300c50fe7ULL, 282, 104 },
	{ 0x850fadc09923329eULL, 309, 112 },
	{ 0xc646d63501a1511eULL, 335, 120 },
	{ 0x93ba47c980e98ce0ULL, 362, 128 },
	{ 0xdc21a1171d42645dULL, 388, 136 },
	{ 0xa402b9c5a8d3a6e7ULL, 415, 144 },
	{ 0xf46518c2ef5b8cd1ULL, 441, 152 },
	{ 0xb616a12b7fe617aaULL, 468, 160 },
	{ 0x87aa9aff79042287ULL, 495, 168 },
	{ 0xca28a291859bbf93ULL, 521, 176 },
	{ 0x969eb7c47859e744ULL, 548, 184 },
	{ 0xe070f78d3927556bULL, 574, 192 },
	{ 0xa738c6bebb12d16dULL, 601, 200 },
	{ 0xf92e0c3537826146ULL, 627, 208 },
	{ 0xb9a74a0637ce2ee1ULL, 654, 216 },
	{ 0x8a5296ffe33cc930ULL, 681, 224 },
	{ 0xce1de40642e3f4b9ULL, 707, 232 },
	{ 0x9991a6f3d6bf1766ULL, 734, 240 },
	{ 0xe4d5e82392a40515ULL, 760, 248 },
	{ 0xaa7eebfb9df9de8eULL, 787, 256 },
	{ 0xfe0efb53d30dd4d8ULL, 813, 264 },
	{ 0xbd49d14aa79dbc82ULL, 840, 272 },
	{ 0x8d07e33455637eb3ULL, 867, 280 },
	{ 0xd226fc195c6a2f8cULL, 893, 288 },
	{ 0x9c935e00d4b9d8d2ULL, 920, 296 },
	{ 0xe950df20247c83fdULL, 946, 304 },
	{ 0xadd57a27d29339f6ULL, 973, 312 },
	{ 0x81842f29f2cce376ULL, 1000, 320 },
	{ 0xc0fe908895cf3b44ULL, 1026, 328 },
	{ 0x8fcac257558ee4e6ULL, 1053, 336 },
	{ 0xd6444e39c3db9b0aULL, 1079, 344 },
	{ 0x9fa42700db900ad2ULL, 1106, 352 },
	{ 0xede24ae798ec8284ULL, 1132, 360 },
	{ 0xb13cc3832ef0c9acULL, 1159, 368 },
	{ 0x840d57e2899d945fULL, 1186, 376 }
};


/**
 * Returns the smallest precomputed power of 10 such that
 *  binary_exp <= power_of_10.bin_exp
 * where
 *  10^decimal_exp = power_of_10.significand * 2^bin_exp
 * with an error of 0.5 ulp in the significand.
 */
void get_power_of_ten(int binary_exp, fp_num_t *power_of_10, int *decimal_exp)
{
	const int powers_count = sizeof(fp_powers_of_10) / sizeof(fp_powers_of_10[0]);
	const int min_bin_exp = fp_powers_of_10[0].bin_exp;
	const int max_bin_exp = fp_powers_of_10[powers_count - 1].bin_exp;
	const int max_bin_exp_diff = 27;

	assert(min_bin_exp <= binary_exp && binary_exp <= max_bin_exp);

	/*
	 * Binary exponent difference between adjacent powers of 10
	 * is lg(10^8) = 26.575. The starting search index seed_idx
	 * undershoots the actual position by less than 1.6%, ie it
	 * skips 26.575/27 = 98.4% of all the smaller powers. This
	 * translates to at most three extra tests.
	 */
	int seed_idx = (binary_exp - min_bin_exp) / max_bin_exp_diff;

	assert(fp_powers_of_10[seed_idx].bin_exp < binary_exp);

	for (int i = seed_idx; i < powers_count; ++i) {
		/* Found the smallest power of 10 with bin_exp >= binary_exp. */
		if (binary_exp <= fp_powers_of_10[i].bin_exp) {
			assert(fp_powers_of_10[i].bin_exp <= binary_exp + max_bin_exp_diff);

			power_of_10->significand = fp_powers_of_10[i].significand;
			power_of_10->exponent = fp_powers_of_10[i].bin_exp;
			*decimal_exp = fp_powers_of_10[i].dec_exp;
			return;
		}
	}

	assert(false);
}

