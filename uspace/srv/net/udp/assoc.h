/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup udp
 * @{
 */
/** @file UDP associations
 */

#ifndef ASSOC_H
#define ASSOC_H

#include <inet/endpoint.h>
#include <ipc/loc.h>
#include "udp_type.h"

extern errno_t udp_assocs_init(udp_assocs_dep_t *);
extern void udp_assocs_fini(void);
extern udp_assoc_t *udp_assoc_new(inet_ep2_t *, udp_assoc_cb_t *, void *);
extern void udp_assoc_delete(udp_assoc_t *);
extern errno_t udp_assoc_add(udp_assoc_t *);
extern void udp_assoc_remove(udp_assoc_t *);
extern void udp_assoc_addref(udp_assoc_t *);
extern void udp_assoc_delref(udp_assoc_t *);
extern void udp_assoc_set_iplink(udp_assoc_t *, service_id_t);
extern errno_t udp_assoc_send(udp_assoc_t *, inet_ep_t *, udp_msg_t *);
extern errno_t udp_assoc_recv(udp_assoc_t *, udp_msg_t **, inet_ep_t *);
extern void udp_assoc_received(inet_ep2_t *, udp_msg_t *);
extern void udp_assoc_reset(udp_assoc_t *);

#endif

/** @}
 */
