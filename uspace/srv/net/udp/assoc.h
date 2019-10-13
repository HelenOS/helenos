/*
 * Copyright (c) 2015 Jiri Svoboda
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
