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
 *  General CRC and checksum computation implementation.
 */

#include <sys/types.h>

#include "include/checksum.h"

/** Big-endian encoding CRC divider.
 */
#define CRC_DIVIDER_BE	0x04C11DB7

/** Little-endian encoding CRC divider.
 */
#define CRC_DIVIDER_LE	0xEDB88320

/** IP checksum value for computed zero checksum.
 *  Zero is returned as 0xFFFF (not flipped)
 */
#define IP_CHECKSUM_ZERO			0xFFFFu

uint32_t compute_crc32_le( uint32_t seed, uint8_t * data, size_t length ){
	size_t	index;

	while( length >= 8 ){
		seed ^= ( * data );
		for( index = 0; index < 8; ++ index ){
			if( seed & 1 ){
				seed = ( seed >> 1 ) ^ (( uint32_t ) CRC_DIVIDER_LE );
			}else{
				seed >>= 1;
			}
		}
		++ data;
		length -= 8;
	}
	if( length > 0 ){
		seed ^= ( * data ) >> ( 8 - length );
		for( index = 0; index < length; ++ index ){
			if( seed & 1 ){
				seed = ( seed >> 1 ) ^ (( uint32_t ) CRC_DIVIDER_LE );
			}else{
				seed >>= 1;
			}
		}
		length -= 8;
	}
	return seed;
}

uint32_t compute_crc32_be( uint32_t seed, uint8_t * data, size_t length ){
	size_t	index;

	while( length >= 8 ){
		seed ^= ( * data ) << 24;
		for( index = 0; index < 8; ++ index ){
			if( seed & 0x80000000 ){
				seed = ( seed << 1 ) ^ (( uint32_t ) CRC_DIVIDER_BE );
			}else{
				seed <<= 1;
			}
		}
		++ data;
		length -= 8;
	}
	if( length > 0 ){
		seed ^= (( * data ) & ( 0xFF << ( 8 - length ))) << 24;
		for( index = 0; index < length; ++ index ){
			if( seed & 0x80000000 ){
				seed = ( seed << 1 ) ^ (( uint32_t ) CRC_DIVIDER_BE );
			}else{
				seed <<= 1;
			}
		}
		length -= 8;
	}
	return seed;
}

uint32_t compute_checksum( uint32_t seed, uint8_t * data, size_t length ){
	size_t	index;

	// sum all the 16 bit fields
	for( index = 0; index + 1 < length; index += 2 ){
		seed += ( data[ index ] << 8 ) + data[ index + 1 ];
	}

	// last odd byte with zero padding
	if( index + 1 == length ){
		seed += data[ index ] << 8;
	}

	return seed;
}

uint16_t compact_checksum( uint32_t sum ){
	// shorten to the 16 bits
	while( sum >> 16 ) sum = ( sum & 0xFFFF ) + ( sum >> 16 );

	return ( uint16_t ) sum;
}

uint16_t flip_checksum( uint16_t checksum ){
	// flip, zero is returned as 0xFFFF (not flipped)
	return ( ~ checksum ) ? ( uint16_t ) ( ~ checksum ) : IP_CHECKSUM_ZERO;
}

uint16_t ip_checksum( uint8_t * data, size_t length ){
	return flip_checksum( compact_checksum( compute_checksum( 0, data, length )));
}

/** @}
 */
