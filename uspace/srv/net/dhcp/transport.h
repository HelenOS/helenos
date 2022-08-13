/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
