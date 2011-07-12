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
 *  @{
 */

#ifndef LIBNET_ICMP_REMOTE_H_
#define LIBNET_ICMP_REMOTE_H_

#include <net/socket_codes.h>
#include <sys/types.h>
#include <net/device.h>
#include <adt/measured_strings.h>
#include <net/packet.h>
#include <net/inet.h>
#include <net/ip_codes.h>
#include <net/icmp_codes.h>
#include <net/icmp_common.h>
#include <async.h>

/** @name ICMP module interface
 * This interface is used by other modules.
 */
/*@{*/

extern int icmp_destination_unreachable_msg(async_sess_t *, icmp_code_t,
    icmp_param_t, packet_t *);
extern int icmp_source_quench_msg(async_sess_t *, packet_t *);
extern int icmp_time_exceeded_msg(async_sess_t *, icmp_code_t, packet_t *);
extern int icmp_parameter_problem_msg(async_sess_t *, icmp_code_t, icmp_param_t,
    packet_t *);

/*@}*/

#endif

/** @}
 */
