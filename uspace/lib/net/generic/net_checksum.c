/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup libnet
 * @{
 */

/** @file
 * General CRC and checksum computation implementation.
 */

#include <sys/types.h>

#include <net_checksum.h>

/** Big-endian encoding CRC divider. */
#define CRC_DIVIDER_BE  0x04c11db7

/** Little-endian encoding CRC divider. */
#define CRC_DIVIDER_LE  0xedb88320

/** Polynomial used in multicast address hashing */
#define CRC_MCAST_POLYNOMIAL  0x04c11db6

/** Compacts the computed checksum to the 16 bit number adding the carries.
 *
 * @param[in] sum	Computed checksum.
 * @return		Compacted computed checksum to the 16 bits.
 */
uint16_t compact_checksum(uint32_t sum)
{
	/* Shorten to the 16 bits */
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return (uint16_t) sum;
}

/** Computes sum of the 2 byte fields.
 *
 * Padds one zero (0) byte if odd.
 *
 * @param[in] seed	Initial value. Often used as 0 or ~0.
 * @param[in] data	Pointer to the beginning of data to process.
 * @param[in] length	Length of the data in bytes.
 * @return		The computed checksum of the length bytes of the data.
 */
uint32_t compute_checksum(uint32_t seed, uint8_t *data, size_t length)
{
	size_t index;

	/* Sum all the 16 bit fields */
	for (index = 0; index + 1 < length; index += 2)
		seed += (data[index] << 8) + data[index + 1];

	/* Last odd byte with zero padding */
	if (index + 1 == length)
		seed += data[index] << 8;

	return seed;
}

/** Computes CRC32 value in the big-endian environment.
 *
 * @param[in] seed	Initial value. Often used as 0 or ~0.
 * @param[in] data	Pointer to the beginning of data to process.
 * @param[in] length	Length of the data in bits.
 * @return		The computed CRC32 of the length bits of the data.
 */
uint32_t compute_crc32_be(uint32_t seed, uint8_t * data, size_t length)
{
	size_t index;

	/* Process full bytes */
	while (length >= 8) {
		/* Add the data */
		seed ^= (*data) << 24;
		
		/* For each added bit */
		for (index = 0; index < 8; ++index) {
			/* If the first bit is set */
			if (seed & 0x80000000) {
				/* Shift and divide the checksum */
				seed = (seed << 1) ^ ((uint32_t) CRC_DIVIDER_BE);
			} else {
				/* Shift otherwise */
				seed <<= 1;
			}
		}
		
		/* Move to the next byte */
		++data;
		length -= 8;
	}

	/* Process the odd bits */
	if (length > 0) {
		/* Add the data with zero padding */
		seed ^= ((*data) & (0xff << (8 - length))) << 24;
		
		/* For each added bit */
		for (index = 0; index < length; ++index) {
			/* If the first bit is set */
			if (seed & 0x80000000) {
				/* Shift and divide the checksum */
				seed = (seed << 1) ^ ((uint32_t) CRC_DIVIDER_BE);
			} else {
				/* Shift otherwise */
				seed <<= 1;
			}
		}
	}

	return seed;
}

/** Computes CRC32 value in the little-endian environment.
 *
 * @param[in] seed	Initial value. Often used as 0 or ~0.
 * @param[in] data	Pointer to the beginning of data to process.
 * @param[in] length	Length of the data in bits.
 * @return		The computed CRC32 of the length bits of the data.
 */
uint32_t compute_crc32_le(uint32_t seed, uint8_t * data, size_t length)
{
	size_t index;

	/* Process full bytes */
	while (length >= 8) {
		/* Add the data */
		seed ^= (*data);
		
		/* For each added bit */
		for (index = 0; index < 8; ++index) {
			/* If the last bit is set */
			if (seed & 1) {
				/* Shift and divide the checksum */
				seed = (seed >> 1) ^ ((uint32_t) CRC_DIVIDER_LE);
			} else {
				/* Shift otherwise */
				seed >>= 1;
			}
		}
		
		/* Move to the next byte */
		++data;
		length -= 8;
	}

	/* Process the odd bits */
	if (length > 0) {
		/* Add the data with zero padding */
		seed ^= (*data) >> (8 - length);
		
		for (index = 0; index < length; ++index) {
			/* If the last bit is set */
			if (seed & 1) {
				/* Shift and divide the checksum */
				seed = (seed >> 1) ^ ((uint32_t) CRC_DIVIDER_LE);
			} else {
				/* Shift otherwise */
				seed >>= 1;
			}
		}
	}

	return seed;
}

/** Returns or flips the checksum if zero.
 *
 * @param[in] checksum	The computed checksum.
 * @return		The internet protocol header checksum.
 * @return		0xFFFF if the computed checksum is zero.
 */
uint16_t flip_checksum(uint16_t checksum)
{
	/* Flip, zero is returned as 0xFFFF (not flipped) */
	checksum = ~checksum;
	return checksum ? checksum : IP_CHECKSUM_ZERO;
}

/** Compute the IP header checksum.
 *
 * To compute the checksum of a new packet, the checksum header field must be
 * zero. To check the checksum of a received packet, the checksum may be left
 * set. Zero will be returned in this case if valid.
 *
 * @param[in] data	The header data.
 * @param[in] length	The header length in bytes.
 * @return		The internet protocol header checksum.
 * @return		0xFFFF if the computed checksum is zero.
 */
uint16_t ip_checksum(uint8_t *data, size_t length)
{
	/* Compute, compact and flip the data checksum */
	return flip_checksum(compact_checksum(compute_checksum(0, data,
	    length)));
}

/** Compute the standard hash from MAC
 *
 * Hashing MAC into 64 possible values and using the value as index to
 * 64bit number.
 *
 * The code is copied from qemu-0.13's implementation of ne2000 and rt8139
 * drivers, but according to documentation there it originates in FreeBSD.
 *
 * @param[in] addr The 6-byte MAC address to be hashed
 *
 * @return 64-bit number with only single bit set to 1
 *
 */
uint64_t multicast_hash(const uint8_t addr[6])
{
	uint32_t crc;
    int carry, i, j;
    uint8_t b;

    crc = 0xffffffff;
    for (i = 0; i < 6; i++) {
        b = addr[i];
        for (j = 0; j < 8; j++) {
            carry = ((crc & 0x80000000L) ? 1 : 0) ^ (b & 0x01);
            crc <<= 1;
            b >>= 1;
            if (carry)
                crc = ((crc ^ CRC_MCAST_POLYNOMIAL) | carry);
        }
    }
	
    uint64_t one64 = 1;
    return one64 << (crc >> 26);
}

/** @}
 */
