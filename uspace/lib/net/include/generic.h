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
 * Generic communication interfaces for networking.
 */

#ifndef LIBNET_GENERIC_H_
#define LIBNET_GENERIC_H_

#include <async.h>
#include <ipc/ipc.h>
#include <ipc/services.h>

#include <net/device.h>
#include <adt/measured_strings.h>
#include <net/packet.h>

extern int generic_device_state_msg_remote(int, int, device_id_t, int,
    services_t);
extern int generic_device_req_remote(int, int, device_id_t, int, services_t);
extern int generic_get_addr_req(int, int, device_id_t, measured_string_t **,
    char **);
extern int generic_packet_size_req_remote(int, int, device_id_t,
    packet_dimension_t *);
extern int generic_received_msg_remote(int, int, device_id_t, packet_id_t,
    services_t, services_t);
extern int generic_send_msg_remote(int, int, device_id_t, packet_id_t,
    services_t, services_t);
extern int generic_translate_req(int, int, device_id_t, services_t,
    measured_string_t *, size_t, measured_string_t **, char **);

#endif

/** @}
 */
