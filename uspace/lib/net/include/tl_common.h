/*
 * Copyright (c) 2008 Lukas Mejdrech
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
 * Transport layer common functions.
 */

#ifndef LIBNET_TL_COMMON_H_
#define LIBNET_TL_COMMON_H_

#include <ipc/services.h>
#include <net/socket_codes.h>
#include <net/packet.h>
#include <net/device.h>
#include <net/inet.h>
#include <async.h>

/** Device packet dimensions.
 * Maps devices to the packet dimensions.
 * @see device.h
 */
DEVICE_MAP_DECLARE(packet_dimensions, packet_dimension_t);

extern int tl_get_ip_packet_dimension(async_sess_t *, packet_dimensions_t *,
    nic_device_id_t, packet_dimension_t **);
extern int tl_get_address_port(const struct sockaddr *, int, uint16_t *);
extern int tl_update_ip_packet_dimension(packet_dimensions_t *, nic_device_id_t,
    size_t);
extern int tl_set_address_port(struct sockaddr *, int, uint16_t);
extern int tl_prepare_icmp_packet(async_sess_t *, async_sess_t *, packet_t *,
    services_t);
extern int tl_socket_read_packet_data(async_sess_t *, packet_t **, size_t,
    const packet_dimension_t *, const struct sockaddr *, socklen_t);

#endif

/** @}
 */
