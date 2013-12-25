/*
 * Copyright (c) 2012 Jiri Svoboda
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
/** @file UDP user calls
 */

#ifndef UCALL_H
#define UCALL_H

#include <ipc/loc.h>
#include <sys/types.h>
#include "udp_type.h"

extern udp_error_t udp_uc_create(udp_assoc_t **);
extern void udp_uc_set_iplink(udp_assoc_t *, service_id_t);
extern udp_error_t udp_uc_set_foreign(udp_assoc_t *, udp_sock_t *);
extern udp_error_t udp_uc_set_local(udp_assoc_t *, udp_sock_t *);
extern udp_error_t udp_uc_set_local_port(udp_assoc_t *, uint16_t);
extern udp_error_t udp_uc_send(udp_assoc_t *, udp_sock_t *, void *, size_t,
    xflags_t);
extern udp_error_t udp_uc_receive(udp_assoc_t *, void *, size_t, size_t *,
    xflags_t *, udp_sock_t *);
extern void udp_uc_status(udp_assoc_t *, udp_assoc_status_t *);
extern void udp_uc_destroy(udp_assoc_t *);
extern void udp_uc_reset(udp_assoc_t *);

#endif

/** @}
 */
