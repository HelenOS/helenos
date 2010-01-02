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

/** @addtogroup net
 *  @{
 */

/** @file
 *  General CRC and checksum computation.
 */

#ifndef __NET_CHECKSUM_H__
#define __NET_CHECKSUM_H__

#include <byteorder.h>

#include <sys/types.h>

/**	Computes CRC32 value.
 *  @param[in] seed Initial value. Often used as 0 or ~0.
 *  @param[in] data Pointer to the beginning of data to process.
 *  @param[in] length Length of the data in bits.
 *  @returns The computed CRC32 of the length bits of the data.
 */
#ifdef ARCH_IS_BIG_ENDIAN
	#define compute_crc32( seed, data, length )	compute_crc32_be( seed, ( uint8_t * ) data, length )
#else
	#define compute_crc32( seed, data, length )	compute_crc32_le( seed, ( uint8_t * ) data, length )
#endif

/**	Computes CRC32 value in the little-endian environment.
 *  @param[in] seed Initial value. Often used as 0 or ~0.
 *  @param[in] data Pointer to the beginning of data to process.
 *  @param[in] length Length of the data in bits.
 *  @returns The computed CRC32 of the length bits of the data.
 */
uint32_t	compute_crc32_le( uint32_t seed, uint8_t * data, size_t length );

/**	Computes CRC32 value in the big-endian environment.
 *  @param[in] seed Initial value. Often used as 0 or ~0.
 *  @param[in] data Pointer to the beginning of data to process.
 *  @param[in] length Length of the data in bits.
 *  @returns The computed CRC32 of the length bits of the data.
 */
uint32_t	compute_crc32_be( uint32_t seed, uint8_t * data, size_t length );

/** Computes sum of the 2 byte fields.
 *  Padds one zero (0) byte if odd.
 *  @param[in] seed Initial value. Often used as 0 or ~0.
 *  @param[in] data Pointer to the beginning of data to process.
 *  @param[in] length Length of the data in bytes.
 *  @returns The computed checksum of the length bytes of the data.
 */
uint32_t	compute_checksum( uint32_t seed, uint8_t * data, size_t length );

/** Compacts the computed checksum to the 16 bit number adding the carries.
 *  @param[in] sum Computed checksum.
 *  @returns Compacted computed checksum to the 16 bits.
 */
uint16_t	compact_checksum( uint32_t sum );

/** Returns or flips the checksum if zero.
 *  @param[in] checksum The computed checksum.
 *  @returns The internet protocol header checksum.
 *  @returns 0xFFFF if the computed checksum is zero.
 */
uint16_t	flip_checksum( uint16_t checksum );

/** Computes the ip header checksum.
 *  To compute the checksum of a new packet, the checksum header field must be zero.
 *  To check the checksum of a received packet, the checksum may be left set.
 *  The zero (0) value will be returned in this case if valid.
 *  @param[in] data The header data.
 *  @param[in] length The header length in bytes.
 *  @returns The internet protocol header checksum.
 *  @returns 0xFFFF if the computed checksum is zero.
 */
uint16_t ip_checksum( uint8_t * data, size_t length );

#endif

/** @}
 */
