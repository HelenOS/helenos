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
 *  ICMP module interface.
 *  The same interface is used for standalone remote modules as well as for bundle modules.
 *  The standalone remote modules have to be compiled with the icmp_remote.c source file.
 *  The bundle modules with the icmp.c source file.
 */

#ifndef __NET_ICMP_INTERFACE_H__
#define __NET_ICMP_INTERFACE_H__

#include <sys/types.h>

#include "device.h"

#include "../structures/measured_strings.h"
#include "../structures/packet/packet.h"

#include "inet.h"
#include "ip_codes.h"
#include "socket_codes.h"

#include "icmp_codes.h"
#include "icmp_common.h"

/** @name ICMP module interface
 *  This interface is used by other modules.
 */
/*@{*/

/** Sends the Destination Unreachable error notification packet.
 *  Beginning of the packet is sent as the notification packet data.
 *  The source and the destination addresses should be set in the original packet.
 *  @param[in] icmp_phone The ICMP module phone used for (semi)remote calls.
 *  @param[in] code The error specific code.
 *  @param[in] mtu The error MTU value.
 *  @param[in] packet The original packet.
 *  @returns EOK on success.
 *  @returns EPERM if the ICMP error notifications are disabled.
 *  @returns ENOMEM if there is not enough memory left.
 */
int icmp_destination_unreachable_msg(int icmp_phone, icmp_code_t code, icmp_param_t mtu, packet_t packet);

/** Sends the Source Quench error notification packet.
 *  Beginning of the packet is sent as the notification packet data.
 *  The source and the destination addresses should be set in the original packet.
 *  @param[in] icmp_phone The ICMP module phone used for (semi)remote calls.
 *  @param[in] packet The original packet.
 *  @returns EOK on success.
 *  @returns EPERM if the ICMP error notifications are disabled.
 *  @returns ENOMEM if there is not enough memory left.
 */
int icmp_source_quench_msg(int icmp_phone, packet_t packet);

/** Sends the Time Exceeded error notification packet.
 *  Beginning of the packet is sent as the notification packet data.
 *  The source and the destination addresses should be set in the original packet.
 *  @param[in] icmp_phone The ICMP module phone used for (semi)remote calls.
 *  @param[in] code The error specific code.
 *  @param[in] packet The original packet.
 *  @returns EOK on success.
 *  @returns EPERM if the ICMP error notifications are disabled.
 *  @returns ENOMEM if there is not enough memory left.
 */
int icmp_time_exceeded_msg(int icmp_phone, icmp_code_t code, packet_t packet);

/** Sends the Parameter Problem error notification packet.
 *  Beginning of the packet is sent as the notification packet data.
 *  The source and the destination addresses should be set in the original packet.
 *  @param[in] icmp_phone The ICMP module phone used for (semi)remote calls.
 *  @param[in] code The error specific code.
 *  @param[in] pointer The problematic parameter offset.
 *  @param[in] packet The original packet.
 *  @returns EOK on success.
 *  @returns EPERM if the ICMP error notifications are disabled.
 *  @returns ENOMEM if there is not enough memory left.
 */
int icmp_parameter_problem_msg(int icmp_phone, icmp_code_t code, icmp_param_t pointer, packet_t packet);

/*@}*/

#endif

/** @}
 */
