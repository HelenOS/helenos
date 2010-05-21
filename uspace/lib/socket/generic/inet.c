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
 *  Internet protocol address conversion functions implementation.
 */

#include <errno.h>
#include <mem.h>
#include <stdio.h>
#include <str.h>

#include <in.h>
#include <in6.h>
#include <inet.h>
#include <socket_codes.h>

int inet_ntop(uint16_t family, const uint8_t * data, char * address, size_t length){
	if((! data) || (! address)){
		return EINVAL;
	}

	switch(family){
		case AF_INET:
			// check the output buffer size
			if(length < INET_ADDRSTRLEN){
				return ENOMEM;
			}
			// fill the buffer with the IPv4 address
			snprintf(address, length, "%hhu.%hhu.%hhu.%hhu", data[0], data[1], data[2], data[3]);
			return EOK;
		case AF_INET6:
			// check the output buffer size
			if(length < INET6_ADDRSTRLEN){
				return ENOMEM;
			}
			// fill the buffer with the IPv6 address
			snprintf(address, length, "%hhx%hhx:%hhx%hhx:%hhx%hhx:%hhx%hhx:%hhx%hhx:%hhx%hhx:%hhx%hhx:%hhx%hhx", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
			return EOK;
		default:
			return ENOTSUP;
	}
}

int inet_pton(uint16_t family, const char * address, uint8_t * data){
	/** The base number of the values.
	 */
	int base;
	/** The number of bytes per a section.
	 */
	size_t bytes;
	/** The number of bytes of the address data.
	 */
	int count;

	const char * next;
	char * last;
	int index;
	size_t shift;
	unsigned long value;

	if(! data){
		return EINVAL;
	}

	// set the processing parameters
	switch(family){
		case AF_INET:
			count = 4;
			base = 10;
			bytes = 1;
			break;
		case AF_INET6:
			count = 16;
			base = 16;
			bytes = 4;
			break;
		default:
			return ENOTSUP;
	}

	// erase if no address
	if(! address){
		bzero(data, count);
		return ENOENT;
	}

	// process the string from the beginning
	next = address;
	index = 0;
	do{
		// if the actual character is set
		if(next && (*next)){

			// if not on the first character
			if(index){
				// move to the next character
				++ next;
			}

			// parse the actual integral value
			value = strtoul(next, &last, base);
			// remember the last problematic character
			// should be either '.' or ':' but is ignored to be more generic
			next = last;

			// fill the address data byte by byte
			shift = bytes - 1;
			do{
				// like little endian
				data[index + shift] = value;
				value >>= 8;
			}while(shift --);

			index += bytes;
		}else{
			// erase the rest of the address
			bzero(data + index, count - index);
			return EOK;
		}
	}while(index < count);

	return EOK;
}

/** @}
 */
