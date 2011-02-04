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
 * @{
 */

/** @file
 * ICMP module messages.
 * @see icmp_remote.h
 */

#ifndef LIBC_ICMP_MESSAGES_
#define LIBC_ICMP_MESSAGES_

#include <ipc/net.h>
#include <sys/types.h>
#include <sys/time.h>
#include <net/icmp_codes.h>

/** ICMP module messages. */
typedef enum {
	/** Send echo request. @see icmp_echo() */
	NET_ICMP_ECHO = NET_ICMP_FIRST,
	
	/**
	 * Send destination unreachable error message.
	 * @see icmp_destination_unreachable_msg()
	 */
	NET_ICMP_DEST_UNREACH,
	
	/**
	 * Send source quench error message.
	 * @see icmp_source_quench_msg()
	 */
	NET_ICMP_SOURCE_QUENCH,
	
	/**
	 * Send time exceeded error message.
	 * @see icmp_time_exceeded_msg()
	 */
	NET_ICMP_TIME_EXCEEDED,
	
	/**
	 * Send parameter problem error message.
	 * @see icmp_parameter_problem_msg()
	 */
	NET_ICMP_PARAMETERPROB
} icmp_messages_t;

/** @name ICMP specific message parameters definitions */
/*@{*/

/** Return the ICMP code message parameter.
 *
 * @param[in] call Message call structure.
 *
 */
#define ICMP_GET_CODE(call)  ((icmp_code_t) IPC_GET_ARG1(call))

/** Return the ICMP link MTU message parameter.
 *
 * @param[in] call Message call structure.
 *
 */
#define ICMP_GET_MTU(call)  ((icmp_param_t) IPC_GET_ARG3(call))

/** Return the pointer message parameter.
 *
 * @param[in] call Message call structure.
 *
 */
#define ICMP_GET_POINTER(call)  ((icmp_param_t) IPC_GET_ARG3(call))

/** Return the size message parameter.
 *
 * @param[in] call Message call structure.
 *
 */
#define ICMP_GET_SIZE(call)  ((size_t) IPC_GET_ARG1(call))

/** Return the timeout message parameter.
 *
 * @param[in] call Message call structure.
 *
 */
#define ICMP_GET_TIMEOUT(call)  ((suseconds_t) IPC_GET_ARG2(call))

/** Return the time to live message parameter.
 *
 * @param[in] call Message call structure.
 *
 */
#define ICMP_GET_TTL(call)  ((ip_ttl_t) IPC_GET_ARG3(call))

/** Return the type of service message parameter.
 *
 * @param[in] call Message call structure.
 *
 */
#define ICMP_GET_TOS(call)  ((ip_tos_t) IPC_GET_ARG4(call))

/** Return the dont fragment message parameter.
 *
 * @param[in] call Message call structure.
 */
#define ICMP_GET_DONT_FRAGMENT(call)  ((int) IPC_GET_ARG5(call))

/*@}*/

#endif

/** @}
 */
