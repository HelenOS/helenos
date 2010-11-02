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

/** @addtogroup libc
 *  @{
 */

/** @file
 * IP codes and definitions.
 */

#ifndef LIBC_IP_CODES_H_
#define LIBC_IP_CODES_H_

#include <sys/types.h>

/** IP time to live counter type definition. */
typedef uint8_t ip_ttl_t;

/** IP type of service type definition. */
typedef uint8_t ip_tos_t;

/** IP transport protocol type definition. */
typedef uint8_t ip_protocol_t;

/** Default IPVERSION. */
#define IPVERSION	4

/** Default time to live counter. */
#define IPDEFTTL	64

/** @name IP options definitions */
/*@{*/

/** Copy shift. */
#define IPOPT_COPY_SHIFT	7

/** Class shift. */
#define IPOPT_CLASS_SHIFT	5

/** Number shift. */
#define IPOPT_NUMBER_SHIFT	0

/** Class mask. */
#define IPOPT_CLASS_MASK	0x60

/** Number mask. */
#define IPOPT_NUMBER_MASK	0x1f

/** Copy flag. */
#define IPOPT_COPY		(1 << IPOPT_COPY_SHIFT)

/** Returns IP option type.
 *
 * @param[in] copy	The value indication whether the IP option should be
 *			copied.
 * @param[in] class	The IP option class.
 * @param[in] number	The IP option number.
 */
#define IPOPT_TYPE(copy, class, number) \
	(((copy) & IPOPT_COPY) | ((class) & IPOPT_CLASS_MASK) | \
	(((number) << IPOPT_NUMBER_SHIFT) & IPOPT_NUMBER_MASK))

/** Returns a value indicating whether the IP option should be copied.
 * @param[in] o The IP option.
 */
#define IPOPT_COPIED(o)		((o) & IPOPT_COPY)

/*@}*/

/** @name IP option class definitions */
/*@{*/

/** Control class. */
#define IPOPT_CONTROL		(0 << IPOPT_CLASS_SHIFT)

/*@}*/

/** @name IP option type definitions */
/*@{*/

/** End of list. */
#define IPOPT_END		IPOPT_TYPE(0, IPOPT_CONTROL, 0)

/** No operation. */
#define IPOPT_NOOP		IPOPT_TYPE(0, IPOPT_CONTROL, 1)

/*@}*/

#endif

/** @}
 */
