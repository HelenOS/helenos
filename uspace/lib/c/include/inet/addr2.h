/*
 * Copyright (c) 2013 Jiri Svoboda
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

#ifndef LIBC_INET2_ADDR_H_
#define LIBC_INET2_ADDR_H_

#include <stdint.h>
#include <net/in.h>

#define INET2_ADDR_SIZE  16

/** Node address */
typedef struct {
	uint16_t family;
	uint8_t addr[INET2_ADDR_SIZE];
} inet2_addr_t;

/** Network address */
typedef struct {
	/** Address family */
	uint16_t family;
	
	/** Address */
	uint8_t addr[INET2_ADDR_SIZE];
	
	/** Number of valid bits */
	uint8_t prefix;
} inet2_naddr_t;

extern int inet2_addr_family(const char *, uint16_t *);

extern int inet2_addr_parse(const char *, inet2_addr_t *);
extern int inet2_naddr_parse(const char *, inet2_naddr_t *);

extern int inet2_addr_format(inet2_addr_t *, char **);
extern int inet2_naddr_format(inet2_naddr_t *, char **);

extern int inet2_addr_pack(inet2_addr_t *, uint32_t *);
extern int inet2_naddr_pack(inet2_naddr_t *, uint32_t *, uint8_t *);

extern void inet2_addr_unpack(uint32_t, inet2_addr_t *);
extern void inet2_naddr_unpack(uint32_t, uint8_t, inet2_naddr_t *);

extern int inet2_addr_sockaddr_in(inet2_addr_t *, sockaddr_in_t *);
extern void inet2_naddr_addr(inet2_naddr_t *, inet2_addr_t *);

extern void inet2_addr(inet2_addr_t *, uint8_t, uint8_t, uint8_t, uint8_t);
extern void inet2_naddr(inet2_naddr_t *, uint8_t, uint8_t, uint8_t, uint8_t,
    uint8_t);

extern void inet2_addr_empty(inet2_addr_t *);
extern void inet2_naddr_empty(inet2_naddr_t *);

extern int inet2_addr_compare(inet2_addr_t *, inet2_addr_t *);
extern int inet2_addr_is_empty(inet2_addr_t *);

#endif

/** @}
 */
