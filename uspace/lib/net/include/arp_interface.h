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

#ifndef LIBNET_ARP_INTERFACE_H_
#define LIBNET_ARP_INTERFACE_H_

#include <adt/measured_strings.h>
#include <task.h>
#include <ipc/services.h>
#include <net/device.h>
#include <net/socket.h>
#include <async.h>

/** @name ARP module interface
 * This interface is used by other modules.
 */
/*@{*/

extern int arp_device_req(async_sess_t *, nic_device_id_t, services_t, services_t,
    measured_string_t *);
extern int arp_translate_req(async_sess_t *, nic_device_id_t, services_t,
    measured_string_t *, measured_string_t **, uint8_t **);
extern int arp_clear_device_req(async_sess_t *, nic_device_id_t);
extern int arp_clear_address_req(async_sess_t *, nic_device_id_t, services_t,
    measured_string_t *);
extern int arp_clean_cache_req(async_sess_t *);
extern async_sess_t *arp_connect_module(services_t);

/*@}*/

#endif

/** @}
 */
