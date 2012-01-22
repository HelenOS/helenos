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
 * General CRC and checksum computation.
 */

#ifndef LIBNET_CHECKSUM_H_
#define LIBNET_CHECKSUM_H_

#include <byteorder.h>
#include <sys/types.h>

/** IP checksum value for computed zero checksum.
 *
 * Zero is returned as 0xFFFF (not flipped)
 *
 */
#define IP_CHECKSUM_ZERO  0xffffU

#ifdef __BE__

#define compute_crc32(seed, data, length) \
	compute_crc32_be(seed, (uint8_t *) data, length)

#endif

#ifdef __LE__

#define compute_crc32(seed, data, length) \
	compute_crc32_le(seed, (uint8_t *) data, length)

#endif

extern uint32_t compute_crc32_le(uint32_t, uint8_t *, size_t);
extern uint32_t compute_crc32_be(uint32_t, uint8_t *, size_t);
extern uint32_t compute_checksum(uint32_t, uint8_t *, size_t);
extern uint16_t compact_checksum(uint32_t);
extern uint16_t flip_checksum(uint16_t);
extern uint16_t ip_checksum(uint8_t *, size_t);

#endif

/** @}
 */
