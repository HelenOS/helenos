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
 *  Host - network byte order manipulation functions.
 */

#ifndef __NET_BYTEORDER_H__
#define __NET_BYTEORDER_H__

#include <byteorder.h>
#include <sys/types.h>


/** Converts the given short number (16 bit) from the host byte order to the network byte order (big endian).
 *  @param[in] number The number in the host byte order to be converted.
 *  @returns The number in the network byte order.
 */
#define htons(number)		host2uint16_t_be(number)

/** Converts the given long number (32 bit) from the host byte order to the network byte order (big endian).
 *  @param[in] number The number in the host byte order to be converted.
 *  @returns The number in the network byte order.
 */
#define htonl(number)		host2uint32_t_be(number)

/** Converts the given short number (16 bit) from the network byte order (big endian) to the host byte order.
 *  @param[in] number The number in the network byte order to be converted.
 *  @returns The number in the host byte order.
 */
#define ntohs(number) 	uint16_t_be2host(number)

/** Converts the given long number (32 bit) from the network byte order (big endian) to the host byte order.
 *  @param[in] number The number in the network byte order to be converted.
 *  @returns The number in the host byte order.
 */
#define ntohl(number)		uint32_t_be2host(number)

#endif

/** @}
 */
