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

/** @addtogroup icmp
 *  @{
 */

/** @file
 *  ICMP module common interface.
 */

#ifndef __NET_ICMP_COMMON_H__
#define __NET_ICMP_COMMON_H__

#include <ipc/services.h>

#include <sys/time.h>

/** Default timeout for incoming connections in microseconds.
 */
#define ICMP_CONNECT_TIMEOUT	(1 * 1000 * 1000)

/** Connects to the ICMP module.
 *  @param service The ICMP module service. Ignored parameter.
 *  @param[in] timeout The connection timeout in microseconds. No timeout if set to zero (0).
 *  @returns The ICMP module phone on success.
 *  @returns The ICMP socket identifier if called by the bundle module.
 *  @returns ETIMEOUT if the connection timeouted.
 */
int icmp_connect_module(services_t service, suseconds_t timeout);

#endif

/** @}
 */
