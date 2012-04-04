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

/** @addtogroup tcp
 * @{
 */

/**
 * @file UDP user calls
 */

#include <io/log.h>

//#include "conn.h"
#include "udp_type.h"
#include "ucall.h"

udp_error_t udp_uc_create(udp_assoc_t **assoc)
{
//	udo_assoc_t *nassoc;

	log_msg(LVL_DEBUG, "udp_uc_create()");

	return UDP_EOK;
}

udp_error_t udp_uc_set_foreign(udp_assoc_t *assoc, udp_sock_t *fsock)
{
//	udo_assoc_t *nconn;

	log_msg(LVL_DEBUG, "udp_uc_set_foreign(%p, %p)", assoc, fsock);

	return UDP_EOK;
}

udp_error_t udp_uc_set_local(udp_assoc_t *assoc, udp_sock_t *lsock)
{
//	udo_assoc_t *nconn;

	log_msg(LVL_DEBUG, "udp_uc_set_local(%p, %p)", assoc, lsock);

	return UDP_EOK;
}

udp_error_t udp_uc_send(udp_assoc_t *assoc, udp_sock_t *fsock, void *data,
    size_t size, xflags_t flags)
{
	log_msg(LVL_DEBUG, "%s: udp_uc_send()", assoc->name);
	return UDP_EOK;
}

udp_error_t udp_uc_receive(udp_assoc_t *assoc, void *buf, size_t size,
    size_t *rcvd, xflags_t *xflags, udp_sock_t *fsock)
{
//	size_t xfer_size;

	log_msg(LVL_DEBUG, "%s: udp_uc_receive()", assoc->name);
	return UDP_EOK;
}

void udp_uc_status(udp_assoc_t *assoc, udp_assoc_status_t *astatus)
{
	log_msg(LVL_DEBUG, "udp_uc_status()");
//	cstatus->cstate = conn->cstate;
}

void udp_uc_destroy(udp_assoc_t *assoc)
{
	log_msg(LVL_DEBUG, "udp_uc_delete()");
//	udp_assoc_destroy(assoc);
}

/**
 * @}
 */
