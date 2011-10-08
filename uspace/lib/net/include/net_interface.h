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

#ifndef LIBNET_NET_INTERFACE_H_
#define LIBNET_NET_INTERFACE_H_

#include <ipc/services.h>
#include <net/device.h>
#include <adt/measured_strings.h>
#include <async.h>
#include <devman.h>

/** @name Networking module interface
 * This interface is used by other modules.
 */
/*@{*/

extern int net_get_device_conf_req(async_sess_t *, nic_device_id_t,
    measured_string_t **, size_t, uint8_t **);
extern int net_get_conf_req(async_sess_t *, measured_string_t **, size_t,
    uint8_t **);
extern void net_free_settings(measured_string_t *, uint8_t *);
extern int net_get_devices_req(async_sess_t *, measured_string_t **, size_t *,
    uint8_t **);
extern int net_driver_ready(async_sess_t *, devman_handle_t);
extern async_sess_t *net_connect_module(void);

/*@}*/

#endif

/** @}
 */
