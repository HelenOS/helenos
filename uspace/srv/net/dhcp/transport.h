/*
 * Copyright (c) 2013 Jiri Svoboda
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

/** @addtogroup dhcp
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <inet/udp.h>
#include <ipc/loc.h>
#include <stddef.h>

struct dhcp_transport;
typedef struct dhcp_transport dhcp_transport_t;

typedef void (*dhcp_recv_cb_t)(void *, void *, size_t);

struct dhcp_transport {
	/** UDP */
	udp_t *udp;
	/** UDP association */
	udp_assoc_t *assoc;
	/** Receive callback */
	dhcp_recv_cb_t recv_cb;
	/** Callback argument */
	void *cb_arg;
};

extern errno_t dhcp_transport_init(dhcp_transport_t *, service_id_t,
    dhcp_recv_cb_t, void *);
extern void dhcp_transport_fini(dhcp_transport_t *);
extern errno_t dhcp_send(dhcp_transport_t *dt, void *msg, size_t size);

#endif

/** @}
 */
