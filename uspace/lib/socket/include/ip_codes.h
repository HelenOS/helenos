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

/** @addtogroup ip
 *  @{
 */

/** @file
 *  IP codes and definitions.
 */

#ifndef __NET_IP_CODES_H__
#define __NET_IP_CODES_H__

#include <sys/types.h>

/** IP time to live counter type definition.
 */
typedef uint8_t	ip_ttl_t;

/** IP type of service type definition.
 */
typedef uint8_t	ip_tos_t;

/** IP transport protocol type definition.
 */
typedef uint8_t	ip_protocol_t;

/** Default IPVERSION.
 */
#define IPVERSION	4

/** Maximum time to live counter.
 */
#define MAXTTL		255

/** Default time to live counter.
 */
#define IPDEFTTL	64

/** @name IP type of service definitions
 */
/*@{*/

/** IP TOS mask.
 */
#define IPTOS_TOS_MASK				0x1E

/** Precedence shift.
 */
#define IPTOS_PRECEDENCE_SHIFT		5

/** Delay shift.
 */
#define IPTOS_DELAY_SHIFT			4

/** Throughput shift.
 */
#define IPTOS_THROUGHPUT_SHIFT		3

/** Reliability shift.
 */
#define IPTOS_RELIABILITY_SHIFT		2

/** Cost shift.
 */
#define IPTOS_COST_SHIFT			1

/** Normal delay.
 */
#define IPTOS_NORMALDELAY			(0x0 << IPTOS_DELAY_SHIFT)

/** Low delay.
 */
#define IPTOS_LOWDELAY				(0x1 << IPTOS_DELAY_SHIFT)

/** Normal throughput.
 */
#define IPTOS_NORMALTHROUGHPUT		(0x0 << IPTOS_THROUGHPUT_SHIFT)

/** Throughput.
 */
#define IPTOS_THROUGHPUT			(0x1 << IPTOS_THROUGHPUT_SHIFT)

/** Normal reliability.
 */
#define IPTOS_NORMALRELIABILITY		(0x0 << IPTOS_RELIABILITY_SHIFT)

/** Reliability.
 */
#define IPTOS_RELIABILITY			(0x1 << IPTOS_RELIABILITY_SHIFT)

/** Normal cost.
 */
#define IPTOS_NORMALCOST			(0x0 << IPTOS_COST_SHIFT)

/** Minimum cost.
 */
#define IPTOS_MICNCOST				(0x1 << IPTOS_COST_SHIFT)

/*@}*/

/** @name IP TOS precedence definitions
 */
/*@{*/


/** Precedence mask.
 */
#define IPTOS_PREC_MASK				0xE0

/** Routine precedence.
 */
#define IPTOS_PREC_ROUTINE			(0x0 << IPTOS_PRECEDENCE_SHIFT)

/** Priority precedence.
 */
#define IPTOS_PREC_PRIORITY			(0x1 << IPTOS_PRECEDENCE_SHIFT)

/** Immediate precedence.
 */
#define IPTOS_PREC_IMMEDIATE		(0x2 << IPTOS_PRECEDENCE_SHIFT)

/** Flash precedence.
 */
#define IPTOS_PREC_FLASH			(0x3 << IPTOS_PRECEDENCE_SHIFT)

/** Flash override precedence.
 */
#define IPTOS_PREC_FLASHOVERRIDE	(0x4 << IPTOS_PRECEDENCE_SHIFT)

/** Critical precedence.
 */
#define IPTOS_PREC_CRITIC_ECP		(0x5 << IPTOS_PRECEDENCE_SHIFT)

/** Inter-network control precedence.
 */
#define IPTOS_PREC_INTERNETCONTROL	(0x6 << IPTOS_PRECEDENCE_SHIFT)

/** Network control precedence.
 */
#define IPTOS_PREC_NETCONTROL		(0x7 << IPTOS_PRECEDENCE_SHIFT)

/*@}*/

/** @name IP options definitions
 */
/*@{*/

/** Copy shift.
 */
#define IPOPT_COPY_SHIFT			7

/** Class shift.
 */
#define IPOPT_CLASS_SHIFT			5

/** Number shift.
 */
#define IPOPT_NUMBER_SHIFT			0

/** Class mask.
 */
#define IPOPT_CLASS_MASK			0x60

/** Number mask.
 */
#define IPOPT_NUMBER_MASK			0x1F

/** Copy flag.
 */
#define IPOPT_COPY					(1 << IPOPT_COPY_SHIFT)

/** Returns IP option type.
 *  @param[in] copy The value indication whether the IP option should be copied.
 *  @param[in] class The IP option class.
 *  @param[in] number The IP option number.
 */
#define IPOPT_TYPE(copy, class, number)	(((copy) &IPOPT_COPY) | ((class) &IPOPT_CLASS_MASK) | ((number << IPOPT_NUMBER_SHIFT) &IPOPT_NUMBER_MASK))

/** Returns a value indicating whether the IP option should be copied.
 *  @param[in] o The IP option.
 */
#define	IPOPT_COPIED(o)			((o) &IPOPT_COPY)

/** Returns an IP option class.
 *  @param[in] o The IP option.
 */
#define	IPOPT_CLASS(o)			((o) &IPOPT_CLASS_MASK)

/** Returns an IP option number.
 *  @param[in] o The IP option.
 */
#define	IPOPT_NUMBER(o)			((o) &IPOPT_NUMBER_MASK)

/*@}*/

/** @name IP option class definitions
 */
/*@{*/

/** Control class.
 */
#define	IPOPT_CONTROL				(0 << IPOPT_CLASS_SHIFT)

/** Reserved class 1.
 */
#define	IPOPT_RESERVED1				(1 << IPOPT_CLASS_SHIFT)

/** Measurement class.
 */
#define	IPOPT_MEASUREMENT			(2 << IPOPT_CLASS_SHIFT)

/** Reserved class 2.
 */
#define	IPOPT_RESERVED2				(3 << IPOPT_CLASS_SHIFT)

/*@}*/

/** @name IP option type definitions
 */
/*@{*/

/** End of list.
 */
//#define IPOPT_END_OF_LIST			0x0
#define IPOPT_END					IPOPT_TYPE(0, IPOPT_CONTROL, 0)

/** No operation.
 */
//#define IPOPT_NO_OPERATION		0x1
#define IPOPT_NOOP					IPOPT_TYPE(0, IPOPT_CONTROL, 1)

/** Security.
 */
//#define IPOPT_SECURITY			0x82
#define IPOPT_SEC					IPOPT_TYPE(IPOPT_COPY, IPOPT_CONTROL, 2)

/** Loose source.
 */
//#define IPOPT_LOOSE_SOURCE		0x83
#define IPOPT_LSRR					IPOPT_TYPE(IPOPT_COPY, IPOPT_CONTROL, 3)

/** Strict route.
 */
//#define IPOPT_STRICT_SOURCE		0x89
#define IPOPT_SSRR					IPOPT_TYPE(IPOPT_COPY, IPOPT_CONTROL, 9)

/** Record route.
 */
//#define IPOPT_RECORD_ROUTE		0x07
#define IPOPT_RR					IPOPT_TYPE(IPOPT_COPY, IPOPT_CONTROL, 7)

/** Stream identifier.
 */
//#define IPOPT_STREAM_IDENTIFIER	0x88
#define IPOPT_SID					IPOPT_TYPE(IPOPT_COPY, IPOPT_CONTROL, 8)

/** Stream identifier length.
 */
#define IPOPT_SID_LENGTH			4

/** Internet timestamp.
 */
//#define IPOPT_INTERNET_TIMESTAMP	0x44
#define IPOPT_TIMESTAMP				IPOPT_TYPE(IPOPT_COPY, IPOPT_MEASUREMENT, 4)

/** Commercial IP security option.
 */
#define IPOPT_CIPSO					IPOPT_TYPE(IPOPT_COPY, IPOPT_CONTROL, 5)

/** No operation variant.
 */
#define IPOPT_NOP IPOPT_NOOP

/** End of list variant.
 */
#define IPOPT_EOL IPOPT_END

/** Timestamp variant.
 */
#define IPOPT_TS  IPOPT_TIMESTAMP

/*@}*/

/** @name IP security option definitions
 */
/*@{*/

/** Security length.
 */
#define IPOPT_SEC_LENGTH			11

/** Unclasified.
 */
#define IPOPT_SEC_UNCLASIFIED		0x0

/** Confidential.
 */
#define IPOPT_SEC_CONFIDENTIAL		0xF035

/** EFTO.
 */
#define IPOPT_SEC_EFTO				0x789A

/** MMMM.
 */
#define IPOPT_SEC_MMMM				0xBC4D

/** PROG.
 */
#define IPOPT_SEC_PROG				0x5E26

/** Restricted.
 */
#define IPOPT_SEC_RESTRICTED		0xAF13

/** Secret.
 */
#define IPOPT_SEC_SECRET			0xD788

/** Top secret.
 */
#define IPOPT_SEC_TOP_SECRET		0x6BC5

/*@}*/

/** @name IP timestamp option definitions
 */
/*@{*/

/** Tiemstamp only.
 */
#define	IPOPT_TS_TSONLY		0

/** Timestamps and addresses.
 */
#define	IPOPT_TS_TSANDADDR	1

/** Specified modules only.
 */
#define	IPOPT_TS_PRESPEC	3

/*@}*/

#endif

/** @}
 */
